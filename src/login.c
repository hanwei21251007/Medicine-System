#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "../include/structs.h"

// 链表头指针和文件操作函数，定义在 file_io.c
extern StaffNode *staff_list;
extern PatientNode *patient_list;
extern DepartmentNode *dept_list;
extern int generate_medical_id();
extern void save_users_to_file();
extern int is_valid_name(const char *s);
extern int is_valid_password(const char *s);
extern int get_choice(int min_choice, int max_choice);
extern int prompt_yes_no(const char *prompt);

// 前向声明
int patient_register_with_id(const char *id_card_input);

// 当前登录用户（全局）
CurrentUser current_user = {0, 0, "", 0};

// 登录菜单
int show_login_menu()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║        医疗管理系统 - 登录界面          ║\n");
    printf("║                                        ║\n");
    printf("║         请选择您的身份：               ║\n");
    printf("║                                        ║\n");
    printf("║         1. 管理员                      ║\n");
    printf("║         2. 医护人员                    ║\n");
    printf("║         3. 患者                        ║\n");
    printf("║         4. 退出系统                    ║\n");
    printf("║                                        ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n请输入您的选择 (1-4): ");
    return get_choice(1, 4);
}

// 医护人员身份选择子菜单，返回对应 UserRole，取消返回 0
int show_staff_submenu()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║          请选择医护人员身份             ║\n");
    printf("║                                        ║\n");
    printf("║         1. 医生                        ║\n");
    printf("║         2. 药剂师                      ║\n");
    printf("║         3. 病房员工                ║\n");
    printf("║         0. 返回上级菜单                ║\n");
    printf("║                                        ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n请输入您的选择 (0-3): ");
    int c = get_choice(0, 3);
    switch (c)
    {
    case 1: return ROLE_DOCTOR;
    case 2: return ROLE_PHARMACIST;
    case 3: return ROLE_WARD_CLERK;
    default: return 0;
    }
}

// 员工登录（管理员 / 医生 / 药剂师 / 病房员工）
int staff_login(UserRole user_role)
{
    current_user.is_logged_in = 0;

    int id;
    char password[50];

    printf("\n请输入工号: ");
    char id_buf[32];
    scanf("%31s", id_buf);
    getchar();
    id = atoi(id_buf);

    printf("请输入密码: ");
    scanf("%49s", password);
    getchar();

    StaffNode *cur = staff_list;
    while (cur)
    {
        if (cur->id == id &&
            strcmp(cur->password, password) == 0 &&
            cur->role == user_role && cur->is_deleted == 0)
        {
            // 医生登录时额外检查所属科室是否已停用
            if (cur->role == ROLE_DOCTOR)
            {
                DepartmentNode *d = dept_list;
                while (d != NULL)
                {
                    if (d->dept_id == cur->dept_id)
                    {
                        if (d->is_deleted == 1)
                        {
                            printf("\n登录失败！您所属的科室【%s】已被停用，请联系管理员。\n", d->dept_name);
                            printf("按enter键返回...");
                            getchar();
                            return 0;
                        }
                        break;
                    }
                    d = d->next;
                }
            }

            current_user.is_logged_in = 1;
            current_user.user_id      = cur->id;
            strncpy(current_user.user_name, cur->name, sizeof(current_user.user_name) - 1);
            current_user.user_role = cur->role;
            current_user.dept_id   = cur->dept_id;
            current_user.title     = cur->title;

            printf("\n登录成功！欢迎 %s\n", current_user.user_name);
            printf("按enter键继续...");
            getchar();
            return 1;
        }
        cur = cur->next;
    }

    printf("\n登录失败！工号、密码或身份错误。\n");
    printf("按enter键返回...");
    getchar();
    return 0;
}

// 患者登录
int patient_login()
{
    current_user.is_logged_in = 0;

    char buf[128];
    char id_card_input[19];
    char password[50];

    printf("\n请输入身份证号: ");
    if (fgets(buf, sizeof(buf), stdin) == NULL)
        return 0;
    if (strchr(buf, '\n') == NULL)
    {
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF)
            ;
    }
    buf[strcspn(buf, "\r\n")] = '\0';

    int i;
    for (i = 0; buf[i] != '\0'; i++)
    {
        if (buf[i] == ' ' || buf[i] == '\t')
        {
            printf("\n身份证号不能含有空格，请重新输入。\n");
            printf("按enter键返回...");
            getchar();
            return 0;
        }
    }

    int id_len = (int)strlen(buf);
    if (id_len != 18)
    {
        printf("\n身份证号长度错误（当前%d位），必须为18位。\n", id_len);
        printf("按enter键返回...");
        getchar();
        return 0;
    }

    for (i = 0; i < 17; i++)
    {
        if (buf[i] < '0' || buf[i] > '9')
        {
            printf("\n身份证号格式错误，前17位必须全为数字（第%d位非法字符'%c'）。\n",
                   i + 1, buf[i]);
            printf("按enter键返回...");
            getchar();
            return 0;
        }
    }

    char last = buf[17];
    if (last != 'X' && last != 'x' && (last < '0' || last > '9'))
    {
        printf("\n身份证号格式错误，最后一位只能为数字或X（当前为'%c'）。\n", last);
        printf("按enter键返回...");
        getchar();
        return 0;
    }

    if (last == 'x')
        buf[17] = 'X';

    strncpy(id_card_input, buf, sizeof(id_card_input) - 1);
    id_card_input[18] = '\0';

    PatientNode *cur = patient_list;
    while (cur)
    {
        if (strcmp(cur->id_card, id_card_input) == 0)
            break;
        cur = cur->next;
    }

    if (!cur)
    {
        if (prompt_yes_no("未检测到注册信息，是否立即注册？"))
        {
            patient_register_with_id(id_card_input);
        }
        return 0;
    }

    printf("请输入密码: ");
    scanf("%49s", password);
    getchar();

    if (strcmp(cur->password, password) == 0 && cur->is_deleted == 0)
    {
        current_user.is_logged_in = 1;
        current_user.user_id      = cur->medical_id;
        strncpy(current_user.user_name, cur->name, sizeof(current_user.user_name) - 1);
        current_user.user_role = ROLE_PATIENT;

        printf("\n登录成功！欢迎 %s\n", current_user.user_name);
        printf("按enter键继续...");
        getchar();
        return 1;
    }

    printf("\n密码错误或用户已注销，请重试。\n");
    printf("按enter键返回...");
    getchar();
    return 0;
}

// 患者注册
int patient_register_with_id(const char *id_card_input)
{
    char name[50], password[50];
    char id_card[19];
    strncpy(id_card, id_card_input, sizeof(id_card) - 1);
    id_card[sizeof(id_card) - 1] = '\0';

    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              患者注册                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    printf("  身份证号：%s\n\n", id_card);

    PatientNode *cur = patient_list;
    while (cur)
    {
        if (strcmp(cur->id_card, id_card) == 0)
        {
            printf("\n该身份证号已注册，请直接登录。\n");
            printf("按enter键返回...");
            getchar();
            return 0;
        }
        cur = cur->next;
    }

    while (1)
    {
        printf("请输入姓名: ");
        fgets(name, sizeof(name), stdin);
        name[strcspn(name, "\n")] = '\0';
        if (is_valid_name(name))
            break;
        printf("姓名无效，只允许纯中文或英文（单词间可有一个空格）。\n");
    }

    while (1)
    {
        printf("请设置登录密码: ");
        fgets(password, sizeof(password), stdin);
        password[strcspn(password, "\n")] = '\0';
        if (is_valid_password(password))
            break;
        printf("密码无效，不允许包含空格。\n");
    }

    PatientNode *node = (PatientNode *)malloc(sizeof(PatientNode));
    if (!node)
    {
        printf("\n内存分配失败，注册取消。\n");
        printf("按enter键返回...");
        getchar();
        return 0;
    }
    node->is_deleted  = 0;
    node->medical_id  = generate_medical_id();
    strncpy(node->id_card,  id_card,  sizeof(node->id_card)  - 1);
    strncpy(node->name,     name,     sizeof(node->name)     - 1);
    strncpy(node->password, password, sizeof(node->password) - 1);
    node->id_card[sizeof(node->id_card)   - 1] = '\0';
    node->name[sizeof(node->name)         - 1] = '\0';
    node->password[sizeof(node->password) - 1] = '\0';
    node->next = NULL;

    if (!patient_list)
    {
        patient_list = node;
    }
    else
    {
        PatientNode *tail = patient_list;
        while (tail->next)
            tail = tail->next;
        tail->next = node;
    }

    save_users_to_file();

    printf("\n注册成功！\n");
    printf("  您的病历号为：%d\n", node->medical_id);
    printf("  今后登录请使用【身份证号 + 密码】。\n");
    printf("\n按enter键返回登录界面...");
    getchar();
    return 1;
}

// 注销
void user_logout()
{
    system("cls");
    printf("您已成功注销！感谢使用医疗管理系统。\n");
    printf("按enter键返回登录界面...");
    getchar();
    current_user.is_logged_in = 0;
}