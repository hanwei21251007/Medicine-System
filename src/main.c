#include <stdio.h>
#include "../include/structs.h"

// 函数声明，定义在 login.c

int  show_login_menu();
int  staff_login(UserRole user_role);
int  patient_login();
void show_main_menu();
void user_logout();

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

        // 已登录：显示功能菜单
        while (current_user.is_logged_in) {
            show_main_menu();

            int choice;
            scanf("%d", &choice);
            getchar();

            if (choice == 0) {
                user_logout();
                break;
            }

            // 根据角色分发功能
            switch (current_user.user_role) {

                case ROLE_ADMIN:
                    // TODO: 管理员功能模块完成后在此替换
                    printf("\n功能开发中...\n");
                    printf("按任意键继续...");
                    getchar();
                    break;

                case ROLE_DOCTOR:
                    // TODO: 医生功能模块完成后在此替换
                    printf("\n功能开发中...\n");
                    printf("按任意键继续...");
                    getchar();
                    break;

                case ROLE_PHARMACIST:
                    // TODO: 药剂师功能模块完成后在此替换
                    printf("\n功能开发中...\n");
                    printf("按任意键继续...");
                    getchar();
                    break;

                case ROLE_PATIENT:
                    // TODO: 患者功能模块完成后在此替换
                    printf("\n功能开发中...\n");
                    printf("按任意键继续...");
                    getchar();
                    break;

                default:
                    break;
            }
        }
    }

    return 0;
}