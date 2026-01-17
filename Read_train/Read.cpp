#include "Read.h"
#include <thread>
using namespace std;
namespace pegtl = tao::pegtl;
bool t = 1;
inline string file_to_memory(string p)
{
   ifstream ifs(p);

   ifs.seekg(0, ios::end);
   string buffer;
   buffer.resize(ifs.tellg());
   ifs.seekg(0);
   ifs.read(&buffer[0], buffer.size());
   ifs.close();
   return buffer;
}

unordered_map<string, vector<Input_info>> netlist_info;
unordered_map<string, vector<Input_info>>::iterator it_info;
Input_info *it_input;

template <typename Rule>
struct action
{
};

// Read_netlist_file
template <>
struct action<Output>
{
   template <typename ActionInput>
   static void apply(const ActionInput &in)
   {
      it_info = netlist_info.emplace(in.string(), NULL).first;
      // cout << in.string() << endl;
   }
};

template <>
struct action<Input>
{
   template <typename ActionInput>
   static void apply(const ActionInput &in)
   {
      // it_info->second.emplace_back();
      it_input = &(it_info->second.emplace_back());
      it_input->name = in.string();
   }
};

template <>
struct action<Pin_Cap>
{
   template <typename ActionInput>
   static void apply(const ActionInput &in)
   {
      it_input->pin_cap = stof(in.string());
   }
};

// Read_delay_file
bool havedelay = 0;
template <>
struct action<delay_output>
{
   template <typename ActionInput>
   static void apply(const ActionInput &in)
   {
      if (in.string() != "")
      {
         it_info = netlist_info.find(in.string());
         if (it_info != netlist_info.end())
            havedelay = 1;
      }
   }
};
template <>
struct action<delay_input>
{
   template <typename ActionInput>
   static void apply(const ActionInput &in)
   {
      if (havedelay)
      {
         for (auto &input : it_info->second)
         {
            if (in.string() == input.name)
            {
               it_input = &input;
            }
         }
      }
   }
};

template <>
struct action<delay_value>
{
   template <typename ActionInput>
   static void apply(const ActionInput &in)
   {
      if (havedelay)
      {
         if (in.string() != "")
         {
            it_input->delay = stof(in.string());
            havedelay = 0;
         }
      }
   }
};

bool Read_netlist_file(string netlist_file)
{
   // cout << netlist_file << endl;
   auto netlist_buffer{file_to_memory(netlist_file)};
   tao::pegtl::memory_input netlist_in(netlist_buffer, "");

   if (!pegtl::parse<netlist, action>(netlist_in))
      return 1;
   return 0;
}

bool Read_delay_file(string delay_file)
{

   // cout << delay_file << endl;
   auto delay_buffer{file_to_memory(delay_file)};
   tao::pegtl::memory_input delay_in(delay_buffer, "");

   if (!pegtl::parse<delay, action>(delay_in))
      return 1;

   return 0;
}