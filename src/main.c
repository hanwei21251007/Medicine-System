#include <stdio.h>
#include "../include/structs.h"

// 函数声明，定义在 login.c
int  show_login_menu();
int  staff_login(UserRole user_role);
int  patient_login();
void user_logout();

// 函数声明，定义在各模块
void admin_menu(void);
void patient_menu(void);
void doctor_menu(void);
// void doctor_menu(void);      // 队友完成后取消注释
// void pharmacy_menu(void);    // 队友完成后取消注释
// void patient_menu(void);     // 队友完成后取消注释

// 函数声明，定义在 file_io.c
void load_all();
void save_all();
void free_all_lists();

// current_user 定义在 login.c
extern CurrentUser current_user;


int main() {
    // 启动时加载所有数据文件到链表
    load_all();

    while (1) {

        // 未登录：显示身份选择界面
        while (!current_user.is_logged_in) {
            int choice = show_login_menu();

            switch (choice) {
                case 1: staff_login(ROLE_ADMIN);      break;
                case 2: staff_login(ROLE_DOCTOR);     break;
                case 3: staff_login(ROLE_PHARMACIST); break;
                case 4: patient_login();              break;
                case 5:
                    save_all();
                    free_all_lists();
                    printf("感谢使用医疗管理系统，再见！\n");
                    return 0;
                default:
                    printf("\n无效选择，请重新输入。\n");
                    printf("按任意键继续...");
                    getchar();
                    break;
            }
        }

        // 已登录：直接进入对应角色的功能模块
        if (current_user.is_logged_in) {
            switch (current_user.user_role) {
                case ROLE_ADMIN:
                    admin_menu();
                    break;
                case ROLE_DOCTOR:
                    doctor_menu();
                    break;
                case ROLE_PHARMACIST:
                    // pharmacy_menu(); // 队友完成后取消注释
                    printf("\n药剂师模块开发中...\n");
                    printf("按任意键继续...");
                    getchar();
                    user_logout();
                    break;
                case ROLE_PATIENT:
                    patient_menu();
                    break;
                default:
                    break;
            }
            // 模块退出后（选0返回）自动回到登录界面
            if (current_user.is_logged_in) {
                user_logout();
            }
        }
    }

    return 0;
}