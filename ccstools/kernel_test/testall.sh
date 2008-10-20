#! /bin/sh

cd ${0%/*}
export PATH=$PWD:${PATH}

echo "Testing all. (All results are reported)"
newns sakura_filesystem_test
newns sakura_bind_test
newns tomoyo_file_test
newns tomoyo_rewrite_test
newns tomoyo_capability_test
newns tomoyo_signal_test
newns tomoyo_network_test
newns tomoyo_argv0_test
newns tomoyo_env_test
newns tomoyo_new_file_test
newns tomoyo_new_capability_test
newns tomoyo_new_network_test
echo
echo
echo
echo "Testing all. (Only ERRORS are reported)"
newns sakura_filesystem_test | grep -vF OK | grep -F '('
newns sakura_bind_test | grep -vF exhausted | grep -vF Done
newns tomoyo_file_test | grep -vF OK | grep -F '('
newns tomoyo_rewrite_test | grep -vF OK | grep -F '('
newns tomoyo_capability_test | grep -vF OK | grep -F '('
newns tomoyo_signal_test | grep -vF OK | grep -F '('
newns tomoyo_network_test | grep -vF OK | grep -F '('
newns tomoyo_argv0_test | grep -vF OK | grep -F '('
newns tomoyo_env_test | grep -vF OK | grep -F '('
echo
echo
echo
echo "Testing policy I/O.  (Only ERRORS are reported)"
newns tomoyo_bprm_test | grep -vF OK
newns tomoyo_cond_test | grep -vF OK
newns ccs_policy_io_test | grep -vF OK
newns tomoyo_new_file_test | grep -vF OK
newns tomoyo_new_capability_test | grep -vF OK
newns tomoyo_new_network_test | grep -vF OK
newns tomoyo_execute_handler_test | grep -vF OK
