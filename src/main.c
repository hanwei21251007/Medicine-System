#include <stdio.h>
#include "../include/structs.h"

// 函数声明，定义在 login.c
int  show_login_menu();
int  show_staff_submenu();
int  staff_login(UserRole user_role);
int  patient_login();
void user_logout();

// 函数声明，定义在各模块
void admin_menu(void);
void patient_menu(void);
void doctor_menu(void);
void pharmacy_menu(void);
void ward_clerk_menu(void);

// 函数声明，定义在 file_io.c
void load_all();
void save_all();
void free_all_lists();

// current_user 定义在 login.c
extern CurrentUser current_user;


int main()
{
    load_all();

    while (1)
    {
        // 未登录：显示身份选择界面
        while (!current_user.is_logged_in)
        {
            int choice = show_login_menu();

            switch (choice)
            {
            case 1:
                staff_login(ROLE_ADMIN);
                break;

            case 2:
            {
                // 医护人员：弹出子菜单选具体身份
                int role = show_staff_submenu();
                if (role != 0)
                    staff_login((UserRole)role);
                break;
            }

            case 3:
                patient_login();
                break;

            case 4:
                save_all();
                free_all_lists();
                printf("感谢使用医疗管理系统，再见！\n");
                return 0;

            default:
                printf("\n无效选择，请重新输入。\n");
                printf("按enter键继续...");
                getchar();
                break;
            }
        }

        // 已登录：进入对应角色功能模块
        if (current_user.is_logged_in)
        {
            switch (current_user.user_role)
            {
            case ROLE_ADMIN:
                admin_menu();
                break;
            case ROLE_DOCTOR:
                doctor_menu();
                break;
            case ROLE_PHARMACIST:
                pharmacy_menu();
                break;
            case ROLE_WARD_CLERK:
                ward_clerk_menu();
                break;
            case ROLE_PATIENT:
                patient_menu();
                break;
            default:
                break;
            }

            // 模块退出后自动回到登录界面
            if (current_user.is_logged_in)
                user_logout();
        }
    }

    return 0;
}