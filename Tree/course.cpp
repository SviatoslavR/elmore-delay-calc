#include <getopt.h>
#include <iostream>
#include <unordered_map>
#include <parser-spef.hpp>
#include <list>
#include <string>
#include <queue>
#include "Read_train/Read.h"
#include <fstream>
#include <thread>
#include <vector>
#include <sstream>
#include <iomanip>
#include <omp.h>
#include <ctime>
#include <deque>
#include <windows.h>
#include <psapi.h>

using namespace std;

extern unordered_map<string, vector<Input_info>> netlist_info;

clock_t t_begin, t_end;

void print_memory_usage(const string& label) {
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        double phys_mem = pmc.WorkingSetSize / (1024.0 * 1024.0);
        double commit_mem = pmc.PagefileUsage / (1024.0 * 1024.0);
        cout << "[" << label << "] Physical: " << phys_mem << " MB, Commit: " << commit_mem << " MB" << endl;
    }
}

// 定义树节点结构体
struct TreeNode {
    string name;
    float cap;
    float downstream_cap;
    float delay;
    bool visited;
    vector<pair<TreeNode*, float>> children;
    vector<pair<TreeNode*, float>> neighbors;

    TreeNode(string n, float c) : name(move(n)), cap(c), downstream_cap(0.0f), delay(0.0f), visited(false) {}
};

// 构建 RC 树
TreeNode* buildRCTree(const spef::Net& net, const string& root_name, unordered_map<string, TreeNode*>& node_map, deque<TreeNode>& node_pool) {
    for (auto& cap : net.caps) {
        const string& node_name = get<0>(cap);
        float capacitance = get<2>(cap);
        node_pool.emplace_back(node_name, capacitance);
        node_map[node_name] = &node_pool.back();
    }

    if (node_map.find(root_name) == node_map.end()) {
        node_pool.emplace_back(root_name, 0.0f);
        node_map[root_name] = &node_pool.back();
    }

    for (auto& res : net.ress) {
        const string& node1 = get<0>(res);
        const string& node2 = get<1>(res);
        float resistance = get<2>(res);
        
        if (node_map.find(node1) == node_map.end()) {
            node_pool.emplace_back(node1, 0.0f);
            node_map[node1] = &node_pool.back();
        }
        if (node_map.find(node2) == node_map.end()) {
            node_pool.emplace_back(node2, 0.0f);
            node_map[node2] = &node_pool.back();
        }

        TreeNode* n1 = node_map[node1];
        TreeNode* n2 = node_map[node2];
        n1->neighbors.push_back({n2, resistance});
        n2->neighbors.push_back({n1, resistance});
    }

    // 3. BFS to build Tree (Children)
    queue<TreeNode*> q;
    TreeNode* root = node_map[root_name];
    
    q.push(root);
    root->visited = true;

    while (!q.empty()) {
        TreeNode* current = q.front();
        q.pop();

        for (auto& neighbor_pair : current->neighbors) {
            TreeNode* next_node = neighbor_pair.first;
            float resistance = neighbor_pair.second;

            if (!next_node->visited) {
                next_node->visited = true;
                q.push(next_node);
                current->children.push_back({next_node, resistance});
            }
        }
    }
    return root;
}

int main(int argc, char **argv)
{
    int c;
    int spef_num;
    char *file_path = nullptr;
    char *feature_path = nullptr;
    int option_index = 0;
    bool print_memory_usage_flag = false;

    static struct option long_options[] = {{"file_path", required_argument, 0, 'f'}, {"feature_path", required_argument, 0, 'e'}, {"spef_num", required_argument, 0, 'm'}, {"memory_usage", no_argument, 0, 's'}};

    while ((c = getopt_long(argc, argv, "s", long_options, &option_index)))
    {
        if (c == -1) break;
        switch (c)
        {
        case 'f':
            file_path = (char *)malloc((strlen(optarg) + 1) * sizeof(char));
            strcpy(file_path, optarg);
            break;
        case 'e':
            feature_path = (char *)malloc((strlen(optarg) + 1) * sizeof(char));
            strcpy(feature_path, optarg);
            break;
        case 'm':
            spef_num = atoi(optarg);
            break;
        case 's':
            print_memory_usage_flag = true;
            break;
        default:
            printf("?? getopt returned character code 0%o ??\n", c);
        }
    }

    // 路径防空
    if (file_path == nullptr) {
        string default_p = "./Data";
        file_path = (char *)malloc((default_p.length() + 1) * sizeof(char));
        strcpy(file_path, default_p.c_str());
    }
    if (feature_path == nullptr) {
        string default_f = "./features";
        feature_path = (char *)malloc((default_f.length() + 1) * sizeof(char));
        strcpy(feature_path, default_f.c_str());
    }

    // 文件读取
    string netlist_file = string(file_path) + "/netlist_info.txt";
    if (Read_netlist_file(netlist_file))
        exit(1);

    string delay_file = string(file_path) + "/delay_data/Group" + to_string(spef_num) + ".txt";
    if (Read_delay_file(delay_file))
    {
        cerr << "读取delay文件失败" << endl;
        exit(1);
    }

    string spef_file = string(file_path) + "/SPEF/Group" + to_string(spef_num) + ".spef";
    spef::Spef parser;
    if (not parser.read(spef_file))
    {
        cerr << "读取spef文件失败:" << *parser.error << endl;
        exit(1);
    }

    printf("Finished reading files.\n");
    // print_memory_usage("After Reading Files");

    // OpenMP 预处理
    vector<spef::Net*> net_ptrs;
    net_ptrs.reserve(parser.nets.size());
    for (auto &net : parser.nets) {
        net_ptrs.push_back(&net);
    }

    // 输出文件
    string output_file = string(feature_path) + "/delay" + to_string(spef_num) + ".txt";
    ofstream outfile(output_file); 
    if (!outfile.is_open()) {
        cerr << "无法打开输出文件: " << output_file << endl;
        exit(1);
    }
    outfile << fixed << setprecision(6);

    // 配置常量
    const float TIME_UNIT_SCALE = 0.001f;
    const float R_DRIVER = 0.0f;

    cout << "Starting parallel processing of " << net_ptrs.size() << " nets..." << endl;

    t_begin = clock();

    // 并行处理
    #pragma omp parallel
    {
        stringstream ss_buffer;
        ss_buffer << fixed << setprecision(6);
        
        deque<TreeNode> node_pool;
        unordered_map<string, TreeNode*> node_map;
        vector<pair<const string*, const Input_info*>> Input;

        #pragma omp for schedule(dynamic)
        for (size_t i = 0; i < net_ptrs.size(); ++i)
        {
            node_pool.clear();
            node_map.clear();
            Input.clear();

            spef::Net* net_ptr = net_ptrs[i];
            auto &net = *net_ptr;

            string out_name;      
            string OUT_REAL_NAME; 
            const vector<Input_info>* paths = nullptr;
            bool flag = 0;

            // 解析连接
            for (auto &connection : net.connections)
            {
                if (connection.direction == spef::ConnectionDirection::OUTPUT)
                {
                    out_name = connection.name;
                    int index = connection.name.rfind(':');
                    int ID = atoi(connection.name.c_str() + 1);
                    
                    if (parser.name_map.count(ID)) {
                        OUT_REAL_NAME = parser.name_map.at(ID) + '/' + connection.name.substr(index + 1, -1);
                    } else {
                        flag = 1; break;
                    }

                    if (netlist_info.find(OUT_REAL_NAME) == netlist_info.end())
                    {
                        flag = 1;
                    }
                    else
                        paths = &netlist_info.at(OUT_REAL_NAME);
                }
            }
            if (flag || out_name.empty()) continue;

            for (auto &connection : net.connections)
            {
                if (connection.direction == spef::ConnectionDirection::INPUT)
                {
                    int index = connection.name.rfind(':');
                    int ID = atoi(connection.name.c_str() + 1);
                    
                    if (parser.name_map.count(ID)) {
                        string Input_Name = parser.name_map.at(ID) + '/' + connection.name.substr(index + 1, -1);

                        for (auto &p : *paths)
                        {
                            if (Input_Name == p.name)
                            {
                                Input.push_back({&connection.name, &p}); // 存入指针
                            }
                        }
                    }
                }
            }

            if (Input.empty()) continue;

            // 构建 RC 树
            TreeNode* root = buildRCTree(net, out_name, node_map, node_pool);
            
            // 添加 Pin 电容
            for (auto& input_pair : Input) {
                const string& spef_id = *input_pair.first;
                if (node_map.count(spef_id)) {
                    node_map[spef_id]->cap += input_pair.second->pin_cap;
                }
            }

            // Two-Pass 算法计算 Elmore 延迟
            auto calc_downstream = [&](auto&& self, TreeNode* node) -> float {
                if (!node) return 0.0f;
                float sum = node->cap;
                for (auto& ch : node->children) sum += self(self, ch.first);
                node->downstream_cap = sum;
                return sum;
            };
            float total_cap = calc_downstream(calc_downstream, root);

            auto calc_delay = [&](auto&& self, TreeNode* node, float delay) -> void {
                if (!node) return;
                node->delay = delay;
                for (auto& ch : node->children) {
                    float drop = ch.second * ch.first->downstream_cap;
                    self(self, ch.first, delay + drop);
                }
            };
            calc_delay(calc_delay, root, R_DRIVER * total_cap);

            // 输出结果
            for (auto& input_pair : Input) {
                const string& spef_id = *input_pair.first;
                const string& real_name = input_pair.second->name;
                
                float computed_val = node_map[spef_id]->delay * TIME_UNIT_SCALE;

                if (computed_val <= 1e-16f) continue;

                ss_buffer << OUT_REAL_NAME << " " << real_name << " " << computed_val << "\n";
            }


        } // end parallel for

        #pragma omp critical
        {
            outfile << ss_buffer.rdbuf();
        }

    }

    t_end = clock();
    double elapsed_secs = double(t_end - t_begin) / CLK_TCK;
    double total_time = double(t_end) / CLK_TCK;

    cout << "Done after " << elapsed_secs << " seconds. Output written to " << output_file << endl;
    cout << "Total time taken: " << total_time << " seconds." << endl;

    if(print_memory_usage_flag) print_memory_usage("Final");

    // 释放内存
    if (file_path) { free(file_path); file_path = nullptr; }
    if (feature_path) { free(feature_path); feature_path = nullptr; }
    
    return 0;
}