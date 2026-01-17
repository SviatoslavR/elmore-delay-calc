import numpy as np
import argparse
import matplotlib.pyplot as plt


parser = argparse.ArgumentParser(description='manual to this script')
parser.add_argument("--delay", type=bool, default=0)  
parser.add_argument("--path", type=str)  # your results
parser.add_argument("--golden", type=str)

args = parser.parse_args()
delay_type = args.delay
self_path = args.path
golden_path = args.golden

goldens = {}
with open(golden_path, 'r') as file:  # golden results
    for line in file:
        lines = line.strip("\n").split(" ")
        goldens[lines[0]+lines[1]] = lines[2]

# print(goldens.values())
test_smaller50_error = []
test_larger50_error = []

golden_list=[]
large_errors = []
with open(self_path, 'r') as file:
    for line in file:
        lines = line.strip("\n").split(' ')
        s = float(lines[2])
        g = float(goldens[lines[0] + lines[1]]) * 1000
        golden_list.append(g)
        signed_error = s - g
        abs_error = abs(signed_error)
        if g > 50:
            test_larger50_error.append(100 * signed_error / g)
            if (100 * abs_error / g)>1e10:
                print('here')
        else:
            test_smaller50_error.append(signed_error)
            if abs_error > 15:
                large_errors.append((lines[0], lines[1], g, s, abs_error))

golden_np= np.asarray(golden_list)
test_smaller50_error = np.array(test_smaller50_error)
test_larger50_error = np.array(test_larger50_error)
print("**************TEST***************")

print("test_smaller50_mean:", np.mean(abs(test_smaller50_error)))
print("test_smaller50_max:", np.max(abs(test_smaller50_error)))
print("test_smaller50_2sigma:", 2*np.var(test_smaller50_error)**0.5)
print("test_smaller50_bias:", np.mean(test_smaller50_error))

print("test_larger50_mean:", np.mean(abs(test_larger50_error)))
print("test_larger50_max:", np.max(abs(test_larger50_error)))
print("test_larger50_2sigma:", 2*np.var(test_larger50_error)**0.5)
print("test_larger50_bias:", np.mean(test_larger50_error))

if large_errors:
    print("\nLarge errors (>15 ps):")
    for output, input_pin, g_val, s_val, err in large_errors:
        print(f"Output: {output}, Input: {input_pin}, Golden: {g_val:.6f}, Computed: {s_val:.6f}, Error: {err:.6f}")
else:
    print("\nNo large errors found.")

# Histogram for errors where golden delay is <= 50ps
plt.figure()
plt.hist(test_smaller50_error, bins=50, facecolor='blue', alpha=0.7)
plt.xlabel('Signed Error (ps)')
plt.ylabel('Frequency')
plt.title('Error Distribution for Golden Delay <= 50ps')
plt.grid(True)
plt.savefig('smaller_50_error_distribution.png')
print("\nSaved histogram to smaller_50_error_distribution.png")

# Histogram for errors where golden delay is > 50ps
plt.figure()
plt.hist(test_larger50_error, bins=50, facecolor='green', alpha=0.7)
plt.xlabel('Signed Relative Error (%)')
plt.ylabel('Frequency')
plt.title('Error Distribution for Golden Delay > 50ps')
plt.grid(True)
plt.savefig('larger_50_error_distribution.png')
print("Saved histogram to larger_50_error_distribution.png")
