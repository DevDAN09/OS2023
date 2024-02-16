#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/uaccess.h>

asmlinkage long sys_string_revert(char __user *user_input, char __user *user_output)
{
    char buffer[100];
    char reversed[100] = {0}; 
    int len, i, j;

    // 유저로부터 온 값을 buffer 로 저장
    if (copy_from_user(buffer, user_input, sizeof(buffer))) {
        return -EFAULT;
    }

    // 아무것도 입력되지 않으면
    if (buffer[0] == '\0') {
        return -EINTR; // 프로그램 종료를 위한 상태 반환
    }

    len = strlen(buffer);

    // 문자열의 역순 만들기
    for(i = len - 1, j = 0; i >= 0; i--, j++) {
        reversed[j] = buffer[i];
    }

    // 계산된 값을 유저에게 전달
    if (copy_to_user(user_output, reversed, strlen(reversed) + 1)) {
        return -EFAULT;
    }

    return 0;
}

SYSCALL_DEFINE2(string_revert, char __user *, user_input, char __user*, user_output){
    return sys_string_revert(user_input, user_output);
}
