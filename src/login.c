#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../include/structs.h"

// 链表头指针和文件操作函数，定义在 file_io.c
extern StaffNode   *staff_list;
extern PatientNode *patient_list;
extern int  generate_medical_id();
extern void save_users_to_file();

// 前向声明
int patient_register_with_id(const char *id_card_input);

// 当前登录用户（全局）
CurrentUser current_user = {0, 0, "", 0};


// 登录菜单

int show_login_menu() {
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║        医疗管理系统 - 登录界面           ║\n");
    printf("║                                        ║\n");
    printf("║         请选择您的身份：                ║\n");
    printf("║                                        ║\n");
    printf("║         1. 管理员                       ║\n");
    printf("║         2. 医生                         ║\n");
    printf("║         3. 药剂师                       ║\n");
    printf("║         4. 患者                         ║\n");
    printf("║         5. 退出系统                     ║\n");
    printf("║                                        ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n请输入您的选择 (1-5): ");

    int choice;
    scanf("%d", &choice);
    getchar();
    return choice;
}


// 员工登录（管理员 / 医生 / 药剂师）

int staff_login(UserRole user_role) {
    current_user.is_logged_in = 0;

    int  id;
    char password[50];

    printf("\n请输入工号: ");
    scanf("%d", &id);
    getchar();

    printf("请输入密码: ");
    scanf("%49s", password);
    getchar();

    StaffNode *cur = staff_list;
    while (cur) {
        if (cur->id == id                          &&
            strcmp(cur->password, password) == 0   &&
            cur->role == user_role) {

            current_user.is_logged_in = 1;
            current_user.user_id      = cur->id;
            strncpy(current_user.user_name, cur->name,
                    sizeof(current_user.user_name) - 1);
            current_user.user_role = cur->role;

            printf("\n登录成功！欢迎 %s\n", current_user.user_name);
            printf("按任意键继续...");
            getchar();
            return 1;
        }
        cur = cur->next;
    }

    printf("\n登录失败！工号、密码或身份错误。\n");
    printf("按任意键返回...");
    getchar();
    return 0;
}


// 患者登录
// 先判断身份证号是否注册，未注册则引导注册，已注册则验证密码
// 验证通过后只将 medical_id 存入 current_user，身份证号不进入全局状态

int patient_login() {
    current_user.is_logged_in = 0;

    char id_card_input[19];
    char password[50];

    printf("\n请输入身份证号: ");
    scanf("%18s", id_card_input);
    getchar();

    // 查链表判断是否注册过
    PatientNode *cur = patient_list;
    while (cur) {
        if (strcmp(cur->id_card, id_card_input) == 0) break;
        cur = cur->next;
    }

    // 未找到 → 引导注册
    if (!cur) {
        printf("\n未检测到注册信息，是否立即注册？(y/n): ");
        char choice;
        scanf("%c", &choice);
        getchar();
        if (choice == 'y' || choice == 'Y') {
            patient_register_with_id(id_card_input);
        }
        return 0;
    }

    // 已注册 → 验证密码
    printf("请输入密码: ");
    scanf("%49s", password);
    getchar();

    if (strcmp(cur->password, password) == 0) {
        current_user.is_logged_in = 1;
        current_user.user_id      = cur->medical_id;
        strncpy(current_user.user_name, cur->name,
                sizeof(current_user.user_name) - 1);
        current_user.user_role = ROLE_PATIENT;

        printf("\n登录成功！欢迎 %s\n", current_user.user_name);
        printf("按任意键继续...");
        getchar();
        return 1;
    }

    printf("\n密码错误，请重试。\n");
    printf("按任意键返回...");
    getchar();
    return 0;
}


// 患者注册
// 接收已输入的身份证号，补充姓名和密码，生成病历号后写回文件

int patient_register_with_id(const char *id_card_input) {
    char name[50], password[50];
    char id_card[19];
    strncpy(id_card, id_card_input, sizeof(id_card) - 1);
    id_card[sizeof(id_card) - 1] = '\0';

    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              患者注册                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    printf("  身份证号：%s\n\n", id_card);

    // 检查是否已注册（正常不会走到这里，防御性保留）
    PatientNode *cur = patient_list;
    while (cur) {
        if (strcmp(cur->id_card, id_card) == 0) {
            printf("\n该身份证号已注册，请直接登录。\n");
            printf("按任意键返回...");
            getchar();
            return 0;
        }
        cur = cur->next;
    }

    printf("请输入姓名: ");
    scanf("%49s", name);
    getchar();

    printf("请设置登录密码: ");
    scanf("%49s", password);
    getchar();

    // 创建新节点
    PatientNode *node = (PatientNode *)malloc(sizeof(PatientNode));
    if (!node) {
        printf("\n内存分配失败，注册取消。\n");
        printf("按任意键返回...");
        getchar();
        return 0;
    }

    node->medical_id = generate_medical_id();
    strncpy(node->id_card,  id_card,  sizeof(node->id_card)  - 1);
    strncpy(node->name,     name,     sizeof(node->name)     - 1);
    strncpy(node->password, password, sizeof(node->password) - 1);
    node->id_card[sizeof(node->id_card)   - 1] = '\0';
    node->name[sizeof(node->name)         - 1] = '\0';
    node->password[sizeof(node->password) - 1] = '\0';
    node->next = NULL;

    // 追加到链表尾
    if (!patient_list) {
        patient_list = node;
    } else {
        PatientNode *tail = patient_list;
        while (tail->next) tail = tail->next;
        tail->next = node;
    }

    // 立即持久化
    save_users_to_file();

    printf("\n注册成功！\n");
    printf("  您的病历号为：%d\n", node->medical_id);
    printf("  今后登录请使用【身份证号 + 密码】。\n");
    printf("\n按任意键返回登录界面...");
    getchar();
    return 1;
}


// 主菜单（根据角色显示对应菜单）

void show_main_menu() {
    system("cls");

    printf("╔════════════════════════════════════════╗\n");
    printf("║  欢迎，%-16s                ║\n", current_user.user_name);
    printf("╚════════════════════════════════════════╝\n\n");

    switch (current_user.user_role) {

        case ROLE_ADMIN:
            printf("┌─ 管理员菜单 ─────────────────────────┐\n");
            printf("│ 1. 科室管理                            │\n");
            printf("│ 2. 药品管理                            │\n");
            printf("│ 3. 医生管理                            │\n");
            printf("│ 4. 患者信息查询                        │\n");
            printf("│ 5. 统计报表                            │\n");
            printf("│ 0. 注销登录                            │\n");
            printf("└────────────────────────────────────────┘\n");
            break;

        case ROLE_DOCTOR:
            printf("┌─ 医生菜单 ───────────────────────────┐\n");
            printf("│ 1. 查看候诊患者（按病历号）            │\n");
            printf("│ 2. 接诊患者                            │\n");
            printf("│ 3. 开具处方                            │\n");
            printf("│ 4. 患者历史记录                        │\n");
            printf("│ 0. 注销登录                            │\n");
            printf("└────────────────────────────────────────┘\n");
            break;

        case ROLE_PHARMACIST:
            printf("┌─ 药剂师菜单 ─────────────────────────┐\n");
            printf("│ 1. 待发药处方（按病历号核对）          │\n");
            printf("│ 2. 发药操作                            │\n");
            printf("│ 3. 药品库存查询                        │\n");
            printf("│ 4. 库存预警                            │\n");
            printf("│ 0. 注销登录                            │\n");
            printf("└────────────────────────────────────────┘\n");
            break;

        case ROLE_PATIENT:
            printf("┌─ 患者菜单 ───────────────────────────┐\n");
            printf("│  您的病历号：%-6d                    │\n",
                   current_user.user_id);
            printf("├────────────────────────────────────────┤\n");
            printf("│ 1. 门诊挂号                            │\n");
            printf("│ 2. 查看就诊记录                        │\n");
            printf("│ 3. 查看处方与缴费                      │\n");
            printf("│ 4. 住院信息查询                        │\n");
            printf("│ 0. 注销登录                            │\n");
            printf("└────────────────────────────────────────┘\n");
            break;

        default:
            break;
    }

    printf("\n请输入您的选择: ");
}


// 注销

void user_logout() {
    system("cls");
    printf("您已成功注销！感谢使用医疗管理系统。\n");
    printf("按任意键返回登录界面...");
    getchar();
    current_user.is_logged_in = 0;
}