#! /bin/sh

export PATH=/root/ccstools/kernel_test/:${PATH}

echo "Testing all. (All results are reported)"
newns sakura_trace_test
newns sakura_filesystem_test
newns sakura_capability_test
newns sakura_bind_test
newns tomoyo_file_test
newns tomoyo_rewrite_test
newns tomoyo_capability_test
newns tomoyo_signal_test
newns tomoyo_network_test
newns tomoyo_port_test
newns tomoyo_argv0_test
echo
echo
echo
echo "Testing all. (Only ERRORS are reported)"
newns sakura_trace_test | grep -vF OK | grep -vF 'ReadOnly:'
newns sakura_filesystem_test | grep -vF OK | grep -F '('
newns sakura_capability_test | grep -vF OK | grep -F '('
newns sakura_bind_test | grep -vF exhausted | grep -vF Done
newns tomoyo_file_test | grep -vF OK | grep -F '('
newns tomoyo_rewrite_test | grep -vF OK | grep -F '('
newns tomoyo_capability_test | grep -vF OK | grep -F '('
newns tomoyo_signal_test | grep -vF OK | grep -F '('
newns tomoyo_network_test | grep -vF OK | grep -F '('
newns tomoyo_port_test | grep -vF OK | grep -F '('
newns tomoyo_argv0_test | grep -vF OK | grep -F '('
