#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

asmlinkage long sys_plus_revert(char __user *user_input, char __user *user_output)
{
    char buffer[100];
    long int number1, number2, result;
    char operation;

    if (copy_from_user(buffer, user_input, sizeof(buffer))) {
        return -EFAULT;
    }

    if (buffer[0] == '\0') {
        return -EINTR;
    }

    if (sscanf(buffer, "%ld %c %ld", &number1, &operation, &number2) == 3) {
        if (operation == '+') {
            result = number1 - number2;
        }else {
            return -EINVAL;  // Invalid argument
        }

        snprintf(buffer, sizeof(buffer), "%ld", result);
        if (copy_to_user(user_output, buffer, strlen(buffer) + 1)) {
            return -EFAULT;
        }
        return 0;
    }

    return -EINVAL;  // Invalid argument
}

SYSCALL_DEFINE2(plus_revert, char __user *,user_input, char __user *,user_output){
    return sys_plus_revert(user_input, user_output);
}
