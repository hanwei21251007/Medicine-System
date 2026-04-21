//韩维组长负责
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "../include/structs.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// 外部链表和函数声明
extern StaffNode *staff_list;
extern PatientNode *patient_list;
extern DepartmentNode *dept_list;
extern DrugNode *drug_list;
extern RoomNode *room_list;
extern RegistrationNode *reg_list;
extern ExamOrderNode *exam_order_list;
extern PrescriptionNode *prescription_list;
extern InpatientNode *inpatient_list;
extern CurrentUser current_user;
extern InpatientApplyNode *inpatient_apply_list;

extern void save_all();
extern int generate_apply_id();
extern int generate_inpatient_id();
extern const char *get_title_name(DoctorTitle title);
extern const void clear_input();
extern int is_valid_name(const char *s);
extern int is_valid_password(const char *s);
extern int prompt_yes_no(const char *prompt);
extern void change_password();

// 病房员工函数声明（定义在 ward_clerk.c）
extern void ward_review_apply();
extern void ward_admit_patient();
extern void ward_discharge_patient();
extern void ward_view_inpatients();

// 获取当前时间，格式 YYYY-MM-DD HH:MM:SS
static void get_today(char *buf)
{
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);
    sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
            tm_info->tm_year + 1900,
            tm_info->tm_mon + 1,
            tm_info->tm_mday,
            tm_info->tm_hour,
            tm_info->tm_min,
            tm_info->tm_sec);
}

// 根据科室ID查科室名
static const char *get_dept_name(int dept_id)
{
    DepartmentNode *cur = dept_list;
    while (cur != NULL)
    {
        if (cur->dept_id == dept_id)
            return cur->dept_name;
        cur = cur->next;
    }
    return "未知科室";
}

// 根据病历号查患者姓名
static const char *get_patient_name(int medical_id)
{
    PatientNode *cur = patient_list;
    while (cur != NULL)
    {
        if (cur->medical_id == medical_id)
            return cur->name;
        cur = cur->next;
    }
    return "未知患者";
}

static const char *get_doctor_name(int doctor_id)
{
    StaffNode *cur = staff_list;
    while (cur != NULL)
    {
        if (cur->id == doctor_id)
            return cur->name;
        cur = cur->next;
    }
    return "未知医生";
}

// 根据病房ID查每日费用
static float get_room_fee(int room_id)
{
    RoomNode *cur = room_list;
    while (cur != NULL)
    {
        if (cur->room_id == room_id)
            return cur->daily_fee;
        cur = cur->next;
    }
    return 0.0f;
}

// 动态计算在院费用（每10秒为一个计费周期，不足一个周期按一个周期算）
float calc_current_fee(InpatientNode *ip)
{
    float unit_fee = get_room_fee(ip->room_id);
    struct tm tm_admit = {0};
    sscanf(ip->admit_date, "%d-%d-%d %d:%d:%d",
           &tm_admit.tm_year, &tm_admit.tm_mon, &tm_admit.tm_mday,
           &tm_admit.tm_hour, &tm_admit.tm_min, &tm_admit.tm_sec);
    tm_admit.tm_year -= 1900;
    tm_admit.tm_mon -= 1;
    tm_admit.tm_isdst = -1;
    time_t t_admit = mktime(&tm_admit);
    time_t t_now = time(NULL);
    int periods = (int)(difftime(t_now, t_admit) / 10.0) + 1;
    if (periods < 1)
        periods = 1;
    return unit_fee * (float)periods;
}

// 科室信息查看

void admin_view_departments()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            科室信息                    ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    printf("%-6s %-16s %-10s\n", "科室ID", "科室名称", "挂号基础价");
    printf("----------------------------------------\n");

    DepartmentNode *cur = dept_list;
    while (cur != NULL)
    {
        printf("%-6d %-16s %.1f元\n",
               cur->dept_id, cur->dept_name, cur->base_reg_fee);
        cur = cur->next;
    }

    printf("\n按enter键返回...");
    getchar();
}

// 医生信息查看

void admin_view_doctors()
{
    while (1)
    {
        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║            医生信息查找                ║\n");
        printf("╚════════════════════════════════════════╝\n\n");

        char keyword[64];
        printf("输入医生姓名检索（模糊匹配，输入0返回）: ");
        fgets(keyword, sizeof(keyword), stdin);
        keyword[strcspn(keyword, "\r\n")] = '\0';

        if (strcmp(keyword, "0") == 0)
            return;

        if (strlen(keyword) == 0)
        {
            printf("输入无效，请重新输入。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }

        printf("\n%-8s %-10s %-16s %-12s %-10s\n", "工号", "姓名", "科室", "职称", "状态");
        printf("----------------------------------------------------------\n");

        int match_count = 0;
        StaffNode *cur = staff_list;
        while (cur != NULL)
        {
            if (cur->role == ROLE_DOCTOR && strstr(cur->name, keyword) != NULL)
            {
                printf("%-8d %-10s %-16s %-12s %s\n",
                       cur->id,
                       cur->name,
                       get_dept_name(cur->dept_id),
                       get_title_name(cur->title),
                       cur->is_deleted ? "[已注销]" : "[正常]");
                match_count++;
            }
            cur = cur->next;
        }

        printf("----------------------------------------------------------\n");

        if (match_count == 0)
        {
            printf("未找到匹配的医生，请重新检索。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }

        printf("\n共找到 %d 名医生。\n", match_count);
        printf("\n按enter键继续检索...");
        getchar();
    }
}

// 药品信息管理
// 药品管理子功能
// 生成新的 drug_id（取当前最大值+1）
static int generate_drug_id()
{
    int max_id = 0;
    DrugNode *p = drug_list;
    while (p != NULL)
    {
        if (p->drug_id > max_id)
            max_id = p->drug_id;
        p = p->next;
    }
    return max_id + 1;
}

// 显示所有药品列表
static void list_all_drugs()
{
    printf("\n%-6s %-20s %-8s %-8s %-8s %-8s\n",
           "药品ID", "药品名称", "科室ID", "单价", "库存", "警戒线");
    printf("------------------------------------------------------------\n");
    DrugNode *cur = drug_list;
    int count = 0;
    while (cur != NULL)
    {
        printf("%-6d %-20s %-8d %-8.2f %-8d %-8d\n",
               cur->drug_id, cur->drug_name, cur->dept_id,
               cur->price, cur->stock, cur->warning_line);
        count++;
        cur = cur->next;
    }
    if (count == 0)
        printf("（暂无药品记录）\n");
    printf("------------------------------------------------------------\n");
}

// 1. 添加药品
static void drug_add()
{
    int tmp;

    while (1)
    {
        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║            添加药品                ║\n");
        printf("╚════════════════════════════════════════╝\n\n");

        DrugNode *node = (DrugNode *)malloc(sizeof(DrugNode));
        if (node == NULL)
        {
            printf("内存分配失败。\n");
            printf("按enter键继续...");
            getchar();
            return;
        }
        memset(node, 0, sizeof(DrugNode));

        // 自动生成 drug_id
        node->drug_id = generate_drug_id();
        printf("系统自动生成药品ID：%d\n", node->drug_id);

        // 输入科室ID
        printf("请输入所属科室ID（0表示通用）: ");
        if (scanf("%d", &node->dept_id) != 1 || node->dept_id < 0)
        {
            clear_input();
            printf("输入无效。\n");
            free(node);
            if (!prompt_yes_no("是否重新添加？"))
                return;
            continue;
        }
        clear_input();

        // 检查科室是否存在且未被停用
        if (node->dept_id != 0)
        {
            DepartmentNode *dept = dept_list;
            int dept_found = 0;
            while (dept != NULL)
            {
                if (dept->dept_id == node->dept_id)
                {
                    if (dept->is_deleted == 1)
                    {
                        printf("科室「%s」已停用，无法关联。\n", dept->dept_name);
                        free(node);
                        if (!prompt_yes_no("是否重新添加？"))
                            return;
                        dept_found = -1; // 标记为停用，跳出内层循环后 continue 外层
                        break;
                    }
                    dept_found = 1;
                    break;
                }
                dept = dept->next;
            }
            if (dept_found == 0)
            {
                printf("科室ID %d 不存在。\n", node->dept_id);
                free(node);
                if (!prompt_yes_no("是否重新添加？"))
                    return;
                continue;
            }
            if (dept_found == -1)
                continue;
        }

        // 输入药品名称
        printf("请输入药品名称（不能含空格）: ");
        fgets(node->drug_name, sizeof(node->drug_name), stdin);
        node->drug_name[strcspn(node->drug_name, "\r\n")] = '\0';

        if (strlen(node->drug_name) == 0)
        {
            printf("药品名称不能为空。\n");
            free(node);
            if (!prompt_yes_no("是否重新添加？"))
                return;
            continue;
        }
        if (strchr(node->drug_name, ' ') != NULL)
        {
            printf("药品名称不能包含空格。\n");
            free(node);
            if (!prompt_yes_no("是否重新添加？"))
                return;
            continue;
        }

        // 检查同名药品
        DrugNode *chk = drug_list;
        int dup = 0;
        while (chk != NULL)
        {
            if (strcmp(chk->drug_name, node->drug_name) == 0)
            {
                printf("警告：已存在同名药品「%s」（ID: %d），确认仍要添加？\n",
                       chk->drug_name, chk->drug_id);
                if (!prompt_yes_no("确认添加？"))
                {
                    free(node);
                    dup = -1; // 用户放弃
                }
                dup = (dup == -1) ? -1 : 1; // 1=继续，-1=放弃
                break;
            }
            chk = chk->next;
        }
        if (dup == -1)
        {
            if (!prompt_yes_no("是否重新添加？"))
                return;
            continue;
        }

        // 输入单价
        node->price = -1.0f;
        while (node->price < 0)
        {
            printf("请输入单价（元，>=0）: ");
            if (scanf("%f", &node->price) != 1 || node->price < 0)
            {
                clear_input();
                printf("输入无效，请重新输入。\n");
                node->price = -1.0f;
                continue;
            }
            clear_input();
        }

        // 输入库存
        tmp = -1;
        while (tmp < 0)
        {
            printf("请输入初始库存（>=0）: ");
            if (scanf("%d", &tmp) != 1 || tmp < 0)
            {
                clear_input();
                printf("输入无效，请重新输入。\n");
                tmp = -1;
                continue;
            }
            clear_input();
        }
        node->stock = tmp;

        // 输入警戒线
        tmp = -1;
        while (tmp < 0)
        {
            printf("请输入库存警戒线（>=0）: ");
            if (scanf("%d", &tmp) != 1 || tmp < 0)
            {
                clear_input();
                printf("输入无效，请重新输入。\n");
                tmp = -1;
                continue;
            }
            clear_input();
        }
        node->warning_line = tmp;
        node->next = NULL;

        // 追加到链表末尾
        if (drug_list == NULL)
        {
            drug_list = node;
        }
        else
        {
            DrugNode *tail = drug_list;
            while (tail->next != NULL)
                tail = tail->next;
            tail->next = node;
        }

        save_all();
        printf("\n药品添加成功！（ID: %d，名称: %s，单价: %.2f，库存: %d，警戒线: %d）\n",
               node->drug_id, node->drug_name, node->price,
               node->stock, node->warning_line);
        printf("按enter键继续...");
        getchar();
        return; // 成功完成，退出
    }
}

// 2. 删除药品（从链表中移除节点并 free）
static void drug_delete()
{
    while (1)
    {
        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║            删除药品                    ║\n");
        printf("╚════════════════════════════════════════╝\n\n");

        // 第一步：输入关键词模糊搜索
        char keyword[64];
        printf("输入药品名称关键字搜索（模糊匹配，输入0返回）: ");
        fgets(keyword, sizeof(keyword), stdin);

        keyword[strcspn(keyword, "\r\n")] = '\0';
        if (strcmp(keyword, "0") == 0)
            return;
        if (strlen(keyword) == 0)
        {

            printf("未找到包含 \"%s\" 的药品。\n", keyword);
            printf("按enter键继续...");
            getchar();
            continue;
        }

        // 第二步：列出匹配结果
        printf("\n%-6s %-20s %-8s %-8s %-8s\n",
               "药品ID", "药品名称", "科室ID", "单价", "库存");
        printf("--------------------------------------------------\n");
        int found = 0;
        DrugNode *cur = drug_list;
        while (cur != NULL)
        {
            if (strstr(cur->drug_name, keyword) != NULL)
            {
                printf("%-6d %-20s %-8d %-8.2f %-8d\n",
                       cur->drug_id, cur->drug_name, cur->dept_id,
                       cur->price, cur->stock);
                found++;
            }
            cur = cur->next;
        }
        printf("--------------------------------------------------\n");
        if (found == 0)
        {
            printf("未找到包含 \"%s\" 的药品。\n", keyword);
            printf("按enter键继续...");
            getchar();
            continue;
        }

        // 第三步：输入ID
        printf("输入要删除的药品ID（输入0返回）: ");
        int del_id;
        char check;
        if (scanf("%d%c", &del_id, &check) != 2 || check != '\n')
        {
            clear_input();
            printf("输入无效，请重新输入。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }
        if (del_id == 0)
            return;

        // 第四步：在链表中查找
        DrugNode *prev = NULL;
        cur = drug_list;
        while (cur != NULL)
        {
            if (cur->drug_id == del_id)
                break;
            prev = cur;
            cur = cur->next;
        }

        if (cur == NULL)
        {
            printf("未找到ID为 %d 的药品。\n", del_id);
            printf("按enter键继续...");
            getchar();
            continue;
        }

        // 第五步：确认删除
        printf("\n即将删除药品：%s（ID: %d），确认删除？\n", cur->drug_name, cur->drug_id);
        if (!prompt_yes_no("确认删除？(y/n): "))
        {
            printf("删除已取消。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }

        // 第六步：从链表中摘除节点
        if (prev == NULL)
            drug_list = cur->next;
        else
            prev->next = cur->next;

        printf("药品已删除：%s（ID: %d）\n", cur->drug_name, cur->drug_id);
        free(cur);

        save_all();
        printf("按enter键继续...");
        getchar();
    }
}
// 3. 修改药品信息（单价、库存、警戒线）
static void drug_modify()
{
    while (1)
    {
        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║            修改药品信息                ║\n");
        printf("╚════════════════════════════════════════╝\n\n");

        char keyword[64];
        printf("输入药品名称关键字搜索（模糊匹配，输入0返回）: ");
        fgets(keyword, sizeof(keyword), stdin);
        keyword[strcspn(keyword, "\r\n")] = '\0';

        if (strcmp(keyword, "0") == 0)
            return;
        if (strlen(keyword) == 0)
        {
            printf("输入无效，请重新输入。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }

        // 显示匹配结果
        printf("\n%-6s %-20s %-8s %-8s %-8s %-8s\n",
               "药品ID", "药品名称", "科室ID", "单价", "库存", "警戒线");
        printf("------------------------------------------------------------\n");
        int match_count = 0;
        DrugNode *cur = drug_list;
        while (cur != NULL)
        {
            if (strstr(cur->drug_name, keyword) != NULL)
            {
                printf("%-6d %-20s %-8d %-8.2f %-8d %-8d\n",
                       cur->drug_id, cur->drug_name, cur->dept_id,
                       cur->price, cur->stock, cur->warning_line);
                match_count++;
            }
            cur = cur->next;
        }

        if (match_count == 0)
        {
            printf("未找到匹配的药品，请重新搜索。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }

        // 确定目标药品
        DrugNode *target = NULL;
        if (match_count == 1)
        {
            cur = drug_list;
            while (cur != NULL)
            {
                if (strstr(cur->drug_name, keyword) != NULL)
                {
                    target = cur;
                    break;
                }
                cur = cur->next;
            }
        }
        else
        {
            while (target == NULL)
            {
                printf("\n输入要修改的药品ID（0重新搜索）: ");
                int sel_id;
                if (scanf("%d", &sel_id) != 1)
                {
                    clear_input();
                    printf("输入无效。\n");
                    continue;
                }
                clear_input();
                if (sel_id == 0)
                    break;
                cur = drug_list;
                while (cur != NULL)
                {
                    if (cur->drug_id == sel_id &&
                        strstr(cur->drug_name, keyword) != NULL)
                    {
                        target = cur;
                        break;
                    }
                    cur = cur->next;
                }
                if (target == NULL)
                    printf("未找到该ID对应的药品，请重新输入。\n");
            }
            if (target == NULL)
                continue;
        }

        printf("\n已选中：%s（ID: %d）\n", target->drug_name, target->drug_id);
        printf("------------------------------------------------------------\n");

        // 修改单价
        float new_price = -2.0f;
        while (new_price < -1.0f)
        {
            printf("当前单价: %.2f，新单价（不修改输入-1）: ", target->price);
            if (scanf("%f", &new_price) != 1 || new_price < -1.0f)
            {
                clear_input();
                printf("输入无效。\n");
                new_price = -2.0f;
                continue;
            }
            clear_input();
        }
        if (new_price >= 0)
            target->price = new_price;

        // 修改库存
        int new_stock = -2;
        while (new_stock < -1)
        {
            printf("当前库存: %d，新库存（不修改输入-1）: ", target->stock);
            if (scanf("%d", &new_stock) != 1 || new_stock < -1)
            {
                clear_input();
                printf("输入无效。\n");
                new_stock = -2;
                continue;
            }
            clear_input();
        }
        if (new_stock >= 0)
            target->stock = new_stock;

        // 修改警戒线
        int new_warn = -2;
        while (new_warn < -1)
        {
            printf("当前警戒线: %d，新警戒线（不修改输入-1）: ", target->warning_line);
            if (scanf("%d", &new_warn) != 1 || new_warn < -1)
            {
                clear_input();
                printf("输入无效。\n");
                new_warn = -2;
                continue;
            }
            clear_input();
        }
        if (new_warn >= 0)
            target->warning_line = new_warn;

        save_all();
        printf("\n修改成功！当前信息：%s，单价: %.2f，库存: %d，警戒线: %d\n",
               target->drug_name, target->price,
               target->stock, target->warning_line);

        if (!prompt_yes_no("是否继续修改其他药品？"))
            return;
    }
}

// 药品信息管理主入口
void admin_manage_drugs()
{
    while (1)
    {
        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║            药品信息管理                ║\n");
        printf("╚════════════════════════════════════════╝\n");
        printf("║  1. 添加药品                       ║\n");
        printf("║  2. 删除药品                       ║\n");
        printf("║  3. 修改药品信息                   ║\n");
        printf("║  0. 返回                           ║\n");
        printf("╚════════════════════════════════════╝\n");
        printf("\n请输入您的选择: ");

        int choice;
        if (scanf("%d", &choice) != 1)
        {
            clear_input();
            printf("输入无效，请重新输入。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }
        clear_input();

        if (choice == 0)
            return;

        switch (choice)
        {
        case 1:
            drug_add();
            break;
        case 2:
            drug_delete();
            break;
        case 3:
            drug_modify();
            break;
        default:
            printf("输入无效，请重新输入。\n");
            printf("按enter键继续...");
            getchar();
            break;
        }
    }
}

// 科室信息管理

void admin_manage_departments()
{
    while (1)
    {
        system("cls");
        printf("\n╔══════════════════════════════════╗\n");
        printf("║           科室信息管理           ║\n");
        printf("╠══════════════════════════════════╣\n");
        printf("║  1. 添加新科室                   ║\n");
        printf("║  2. 修改科室状态                 ║\n");
        printf("║  0. 返回                         ║\n");
        printf("╚══════════════════════════════════╝\n");
        printf("\n请输入您的选择: ");

        int choice;
        if (scanf("%d", &choice) != 1)
        {
            clear_input();
            continue;
        }
        clear_input();

        if (choice == 0)
            return;

        // ── 添加新科室 ──────────────────────────────────────────────────
        if (choice == 1)
        {
            system("cls");
            printf("╔════════════════════════════════════════╗\n");
            printf("║              添加新科室                ║\n");
            printf("╚════════════════════════════════════════╝\n\n");

            DepartmentNode *node = (DepartmentNode *)malloc(sizeof(DepartmentNode));
            if (node == NULL)
            {
                printf("内存分配失败。\n");
                printf("按enter键继续...");
                getchar();
                continue;
            }

            // 自动生成科室ID
            int max_id = 0;
            DepartmentNode *p = dept_list;
            while (p != NULL)
            {
                if (p->dept_id > max_id)
                    max_id = p->dept_id;
                p = p->next;
            }
            node->dept_id = max_id + 1;
            node->is_deleted = 0;
            node->next = NULL;
            printf("系统自动生成科室ID：%d\n", node->dept_id);

            // 输入科室名称
            printf("输入科室名称: ");
            fgets(node->dept_name, sizeof(node->dept_name), stdin);
            node->dept_name[strcspn(node->dept_name, "\r\n")] = '\0';
            if (strlen(node->dept_name) == 0)
            {
                printf("科室名称不能为空，已取消。\n");
                free(node);
                printf("按enter键继续...");
                getchar();
                continue;
            }
            if (!is_valid_name(node->dept_name))
            {
                printf("科室名称不能包含特殊字符，已取消。\n");
                free(node);
                printf("按enter键继续...");
                getchar();
                continue;
            }
            // 输入挂号基础价
            float fee = -1.0f;
            while (fee < 0)
            {
                printf("输入挂号基础价（元）: ");
                if (scanf("%f", &fee) != 1 || fee < 0)
                {
                    clear_input();
                    printf("输入无效，请重新输入。\n");
                    fee = -1.0f;
                    continue;
                }
                clear_input();
            }
            node->base_reg_fee = fee;

            // 追加到链表末尾
            if (dept_list == NULL)
            {
                dept_list = node;
            }
            else
            {
                DepartmentNode *tail = dept_list;
                while (tail->next != NULL)
                    tail = tail->next;
                tail->next = node;
            }

            save_all();
            printf("科室添加成功！（ID: %d，名称: %s，挂号价: %.1f元）\n",
                   node->dept_id, node->dept_name, node->base_reg_fee);
            printf("按enter键继续...");
            getchar();
            continue;
        }

        // 修改科室状态
        if (choice == 2)
        {
            system("cls");
            printf("╔════════════════════════════════════════╗\n");
            printf("║              修改科室状态              ║\n");
            printf("╚════════════════════════════════════════╝\n\n");

            // 显示所有科室
            printf("%-6s %-16s %-10s %-8s\n",
                   "科室ID", "科室名称", "挂号基础价", "状态");
            printf("--------------------------------------------\n");
            DepartmentNode *cur = dept_list;
            while (cur != NULL)
            {
                printf("%-6d %-16s %-10.1f %s\n",
                       cur->dept_id, cur->dept_name, cur->base_reg_fee,
                       cur->is_deleted ? "[已停用]" : "[正常]");
                cur = cur->next;
            }

            // 选择要修改的科室
            DepartmentNode *target = NULL;
            while (target == NULL)
            {
                printf("\n输入要修改状态的科室ID (0 返回): ");
                int sel_id;
                if (scanf("%d", &sel_id) != 1)
                {
                    clear_input();
                    printf("输入无效。\n");
                    continue;
                }
                clear_input();
                if (sel_id == 0)
                    break;

                cur = dept_list;
                while (cur != NULL)
                {
                    if (cur->dept_id == sel_id)
                    {
                        target = cur;
                        break;
                    }
                    cur = cur->next;
                }
                if (target == NULL)
                    printf("未找到该科室，请重新输入。\n");
            }
            if (target == NULL)
                continue;

            // 切换状态
            printf("科室：%s（ID: %d），当前状态：%s\n",
                   target->dept_name, target->dept_id,
                   target->is_deleted ? "[已停用]" : "[正常]");

            char msg[128];
            if (target->is_deleted == 0)
                snprintf(msg, sizeof(msg), "确认停用科室 %s？", target->dept_name);
            else
                snprintf(msg, sizeof(msg), "确认启用科室 %s？", target->dept_name);

            if (prompt_yes_no(msg))
            {
                target->is_deleted = target->is_deleted ? 0 : 1;
                save_all();
                printf("科室状态已更新为：%s\n",
                       target->is_deleted ? "[已停用]" : "[正常]");
            }
            printf("按enter键继续...");
            getchar();
            continue;
        }

        printf("输入无效，请重新输入。\n");
        printf("按enter键继续...");
        getchar();
    }
}

// 员工信息管理

void admin_manage_staff()
{
    while (1)
    {
        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║            员工信息管理                ║\n");
        printf("╚════════════════════════════════════════╝\n\n");

        printf("1. 添加新员工\n");
        printf("2. 修改员工状态\n");
        printf("0. 返回\n");
        printf("\n请输入您的选择: ");

        int choice;
        if (scanf("%d", &choice) != 1)
        {
            clear_input();
            printf("输入无效，请重新输入。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }
        clear_input();

        if (choice == 0)
            return;

        if (choice < 1 || choice > 2)
        {
            printf("输入无效，请重新输入。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }

        // 添加新员工
        if (choice == 1)
        {
            StaffNode *node = (StaffNode *)malloc(sizeof(StaffNode));
            if (node == NULL)
            {
                printf("内存分配失败。\n");
                printf("按enter键返回...");
                getchar();
                continue;
            }

            // 自动生成工号
            int max_id = 1000;
            StaffNode *check = staff_list;
            while (check != NULL)
            {
                if (check->id > max_id)
                    max_id = check->id;
                check = check->next;
            }
            node->id = max_id + 1;
            node->is_deleted = 0;
            printf("系统自动生成工号：%d\n", node->id);

            // 输入并验证姓名
            int name_ok = 0;
            while (!name_ok)
            {
                printf("输入姓名（仅限中文或英文）: ");
                fgets(node->name, sizeof(node->name), stdin);
                node->name[strcspn(node->name, "\r\n")] = '\0';
                if (strlen(node->name) == 0)
                {
                    printf("姓名不能为空。\n");
                    if (!prompt_yes_no("是否重试？"))
                    {
                        free(node);
                        break;
                    }
                    continue;
                }
                if (is_valid_name(node->name))
                {
                    name_ok = 1;
                }
                else
                {
                    printf("姓名不合法，仅支持中文或英文。\n");
                    if (!prompt_yes_no("是否重试？"))
                    {
                        free(node);
                        break;
                    }
                }
            }
            if (!name_ok)
                continue;

            printf("输入密码: ");
            fgets(node->password, sizeof(node->password), stdin);
            node->password[strcspn(node->password, "\r\n")] = '\0';

            // 角色选择
            int role = 0;
            int role_ok = 0;
            while (!role_ok)
            {

                printf("选择角色 (1=管理员 2=医生 3=药剂师 4=病房员工): ");
                if (scanf("%d", &role) != 1 || role < 1 || role > 4)
                {
                    clear_input();
                    printf("输入无效。\n");
                    if (!prompt_yes_no("是否重试？"))
                    {
                        free(node);
                        role_ok = -1;
                        break;
                    }
                    role = 0;
                    continue;
                }
                clear_input();
                role_ok = 1;
            }
            if (role_ok != 1)
                continue;

            node->role = (UserRole)role;
            node->dept_id = 0;
            node->title = 0;

            if (role == ROLE_DOCTOR)
            {
                // 科室选择
                int dept_ok = 0;
                while (!dept_ok)
                {
                    printf("输入所属科室ID (1-5): ");
                    if (scanf("%d", &node->dept_id) != 1 ||
                        node->dept_id < 1 || node->dept_id > 5)
                    {
                        clear_input();
                        printf("输入无效。\n");
                        if (!prompt_yes_no("是否重试？"))
                        {
                            free(node);
                            dept_ok = -1;
                            break;
                        }
                        continue;
                    }
                    clear_input();
                    dept_ok = 1;
                }
                if (dept_ok != 1)
                    continue;

                // 职称选择
                int title = 0;
                int title_ok = 0;
                while (!title_ok)
                {
                    printf("选择职称 (1=住院医师 2=主治医师 3=副主任医师 4=主任医师): ");
                    if (scanf("%d", &title) != 1 || title < 1 || title > 4)
                    {
                        clear_input();
                        printf("输入无效。\n");
                        if (!prompt_yes_no("是否重试？"))
                        {
                            free(node);
                            title_ok = -1;
                            break;
                        }
                        title = 0;
                        continue;
                    }
                    clear_input();
                    title_ok = 1;
                }
                if (title_ok != 1)
                    continue;
                node->title = (DoctorTitle)title;
            }

            node->next = NULL;
            if (staff_list == NULL)
            {
                staff_list = node;
            }
            else
            {
                StaffNode *tail = staff_list;
                while (tail->next != NULL)
                    tail = tail->next;
                tail->next = node;
            }

            save_all();
            printf("员工添加成功！\n");
            printf("按enter键返回...");
            getchar();
            continue;
        }

        // 修改员工状态（启用/禁用双向切换）
        if (choice == 2)
        {
            system("cls");
            printf("╔════════════════════════════════════════╗\n");
            printf("║            修改员工状态                ║\n");
            printf("╚════════════════════════════════════════╝\n\n");

            int target_id;
            printf("请输入要修改状态的员工工号: ");
            if (scanf("%d", &target_id) != 1)
            {
                clear_input();
                printf("输入无效。\n");
                printf("按enter键继续...");
                getchar();
                continue;
            }
            clear_input();

            if (target_id == current_user.user_id)
            {
                printf("不能修改当前登录账号的状态。\n");
                printf("按enter键继续...");
                getchar();
                continue;
            }

            StaffNode *cur = staff_list;
            int found = 0;
            while (cur != NULL)
            {
                if (cur->id == target_id)
                {
                    found = 1;
                    // 管理员账号不允许禁用
                    if (cur->role == ROLE_ADMIN && cur->is_deleted == 0)
                    {
                        printf("不能禁用管理员账号。\n");
                        printf("按enter键继续...");
                        getchar();
                        break;
                    }

                    printf("员工：%s（工号%d），当前状态：%s\n",
                           cur->name, cur->id,
                           cur->is_deleted ? "[已禁用]" : "[正常]");

                    if (cur->is_deleted == 0)
                    {
                        // 正常 → 禁用
                        char msg[100];
                        snprintf(msg, sizeof(msg), "确认禁用员工 %s（工号%d）？", cur->name, cur->id);
                        if (prompt_yes_no(msg))
                        {
                            cur->is_deleted = 1;
                            save_all();
                            printf("\n员工 %s 已禁用（逻辑删除）。\n", cur->name);
                            printf("注意：数据保留在系统中，仅标记为已禁用。\n");
                        }
                        else
                        {
                            printf("已取消。\n");
                        }
                    }
                    else
                    {
                        // 已禁用 → 恢复正常
                        char msg[100];
                        snprintf(msg, sizeof(msg), "确认恢复员工 %s（工号%d）的账号？", cur->name, cur->id);
                        if (prompt_yes_no(msg))
                        {
                            cur->is_deleted = 0;
                            save_all();
                            printf("\n员工 %s 已恢复正常状态。\n", cur->name);
                        }
                        else
                        {
                            printf("已取消。\n");
                        }
                    }

                    printf("按enter键继续...");
                    getchar();
                    break;
                }
                cur = cur->next;
            }
            if (!found)
            {
                printf("未找到工号为 %d 的员工。\n", target_id);
                printf("按enter键继续...");
                getchar();
            }
            continue;
        }
    }
}

// 统计报表 - 门诊统计

static void stat_outpatient()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              门诊统计                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    printf("【各科室挂号人数】\n");
    printf("%-16s %-10s\n", "科室名称", "挂号人数");
    printf("----------------------------\n");
    DepartmentNode *dept = dept_list;
    while (dept != NULL)
    {
        int count = 0;
        RegistrationNode *reg = reg_list;
        while (reg != NULL)
        {
            if (reg->dept_id == dept->dept_id && reg->is_cancelled == 0)
                count++;
            reg = reg->next;
        }
        printf("%-16s %-10d\n", dept->dept_name, count);
        dept = dept->next;
    }

    // 统计每位医生的挂号数量并输出前十名
    printf("\n【医生接诊挂号量 Top 10】\n");

    // 先统计医生总数
    int doc_count = 0;
    StaffNode *s = staff_list;
    while (s != NULL)
    {
        if (s->role == ROLE_DOCTOR && s->is_deleted == 0)
            doc_count++;
        s = s->next;
    }

    if (doc_count == 0)
    {
        printf("暂无医生数据。\n");
    }
    else
    {
        int *doc_ids = (int *)malloc(doc_count * sizeof(int));
        char **doc_names = (char **)malloc(doc_count * sizeof(char *));
        int *doc_regs = (int *)malloc(doc_count * sizeof(int));

        if (doc_ids == NULL || doc_names == NULL || doc_regs == NULL)
        {
            printf("内存分配失败。\n");
            free(doc_ids);
            free(doc_names);
            free(doc_regs);
        }
        else
        {
            // 填充医生数据，挂号数初始化为0
            int i = 0;
            s = staff_list;
            while (s != NULL)
            {
                if (s->role == ROLE_DOCTOR && s->is_deleted == 0)
                {
                    doc_ids[i] = s->id;
                    doc_names[i] = s->name;
                    doc_regs[i] = 0;
                    i++;
                }
                s = s->next;
            }

            // 遍历挂号记录，累加各医生挂号量
            RegistrationNode *reg = reg_list;
            while (reg != NULL)
            {
                if (reg->is_cancelled == 0)
                {
                    for (i = 0; i < doc_count; i++)
                    {
                        if (doc_ids[i] == reg->doctor_id)
                        {
                            doc_regs[i]++;
                            break;
                        }
                    }
                }
                reg = reg->next;
            }

            // 冒泡排序，按挂号量从高到低
            for (int a = 0; a < doc_count - 1; a++)
            {
                for (int b = 0; b < doc_count - 1 - a; b++)
                {
                    if (doc_regs[b] < doc_regs[b + 1])
                    {
                        int tmp_id = doc_ids[b];
                        doc_ids[b] = doc_ids[b + 1];
                        doc_ids[b + 1] = tmp_id;
                        int tmp_reg = doc_regs[b];
                        doc_regs[b] = doc_regs[b + 1];
                        doc_regs[b + 1] = tmp_reg;
                        char *tmp_name = doc_names[b];
                        doc_names[b] = doc_names[b + 1];
                        doc_names[b + 1] = tmp_name;
                    }
                }
            }

            // 输出前十名（不足10人则全部输出）
            int top = doc_count < 10 ? doc_count : 10;
            printf("%-6s %-16s %-10s\n", "排名", "医生姓名", "挂号数量");
            printf("----------------------------------\n");
            for (i = 0; i < top; i++)
                printf("%-6d %-16s %-10d\n", i + 1, doc_names[i], doc_regs[i]);

            free(doc_ids);
            free(doc_names);
            free(doc_regs);
        }
    }

    printf("\n按enter键返回...");
    getchar();
}

// 统计报表 - 住院统计

static void stat_inpatient()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              住院统计                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 当前在院患者数量
    printf("【当前在院患者】\n");
    int inpatient_count = 0;
    InpatientNode *ip = inpatient_list;
    while (ip != NULL)
    {
        if (ip->is_discharged == 0)
            inpatient_count++;
        ip = ip->next;
    }
    printf("当前在院患者总数：%d 人\n", inpatient_count);

    // 病房使用情况
    printf("\n【病房使用情况】\n");
    printf("%-8s %-16s %-10s %-8s %-8s\n",
           "病房ID", "科室", "类型", "总床位", "已占用");
    printf("----------------------------------------------------\n");
    RoomNode *room = room_list;
    while (room != NULL)
    {
        char *type_name = "普通多人间";
        if (room->room_type == ROOM_B)
            type_name = "术后观察";
        if (room->room_type == ROOM_C)
            type_name = "急诊留观";
        printf("%-8d %-16s %-10s %-8d %-8d\n",
               room->room_id, get_dept_name(room->dept_id),
               type_name, room->capacity, room->current);
        room = room->next;
    }

    printf("\n按enter键返回...");
    getchar();
}

// 统计报表 - 药品统计

static void stat_drug()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              药品统计                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 统计每种药品的已售数量（STATUS_PENDING_DO + STATUS_DONE 的处方）
    // 先数药品总数
    int drug_count = 0;
    DrugNode *d = drug_list;
    while (d != NULL)
    {
        drug_count++;
        d = d->next;
    }

    if (drug_count == 0)
    {
        printf("暂无药品数据。\n");
        printf("\n按enter键返回...");
        getchar();
        return;
    }

    // 动态分配排名数组
    int *ids = (int *)malloc(drug_count * sizeof(int));
    char **names = (char **)malloc(drug_count * sizeof(char *));
    int *sales = (int *)malloc(drug_count * sizeof(int));

    if (ids == NULL || names == NULL || sales == NULL)
    {
        printf("内存分配失败。\n");
        free(ids);
        free(names);
        free(sales);
        printf("\n按enter键返回...");
        getchar();
        return;
    }

    // 填充药品ID和名称，销量初始化为0
    int i = 0;
    d = drug_list;
    while (d != NULL)
    {
        ids[i] = d->drug_id;
        names[i] = d->drug_name;
        sales[i] = 0;
        i++;
        d = d->next;
    }

    // 遍历处方，累加各药品销量
    PrescriptionNode *p = prescription_list;
    while (p != NULL)
    {
        if (p->status == STATUS_PENDING_DO || p->status == STATUS_DONE)
        {
            for (i = 0; i < drug_count; i++)
            {
                if (ids[i] == p->drug_id)
                {
                    sales[i] += p->quantity;
                    break;
                }
            }
        }
        p = p->next;
    }

    // 冒泡排序，按销量从高到低
    for (int a = 0; a < drug_count - 1; a++)
    {
        for (int b = 0; b < drug_count - 1 - a; b++)
        {
            if (sales[b] < sales[b + 1])
            {
                int tmp_id = ids[b];
                ids[b] = ids[b + 1];
                ids[b + 1] = tmp_id;
                int tmp_sale = sales[b];
                sales[b] = sales[b + 1];
                sales[b + 1] = tmp_sale;
                char *tmp_name = names[b];
                names[b] = names[b + 1];
                names[b + 1] = tmp_name;
            }
        }
    }

    // 输出排名
    printf("【药品销售量排名】\n");
    printf("%-6s %-20s %-10s\n", "排名", "药品名称", "销售量(盒)");
    printf("------------------------------------\n");
    for (i = 0; i < drug_count; i++)
    {
        printf("%-6d %-20s %-10d\n", i + 1, names[i], sales[i]);
    }

    free(ids);
    free(names);
    free(sales);

    printf("\n按enter键返回...");
    getchar();
}

// 统计报表 - 收入统计

static void stat_income()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              收入统计                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    float reg_income = 0.0f;
    RegistrationNode *reg = reg_list;
    while (reg != NULL)
    {
        if ((reg->status == STATUS_PENDING_DO || reg->status == STATUS_DONE) && reg->is_cancelled == 0)
            reg_income += reg->reg_fee;
        reg = reg->next;
    }

    float exam_income = 0.0f;
    ExamOrderNode *exam = exam_order_list;
    while (exam != NULL)
    {
        if (exam->status == STATUS_PENDING_DO || exam->status == STATUS_DONE)
            exam_income += exam->price;
        exam = exam->next;
    }

    float drug_income = 0.0f;
    PrescriptionNode *presc = prescription_list;
    while (presc != NULL)
    {
        if (presc->status == STATUS_PENDING_DO || presc->status == STATUS_DONE)
            drug_income += presc->price;
        presc = presc->next;
    }

    float inpatient_income = 0.0f;
    InpatientNode *inp = inpatient_list;
    while (inp != NULL)
    {
        if (inp->is_discharged)
            inpatient_income += inp->total_fee;
        else
            inpatient_income += calc_current_fee(inp);
        inp = inp->next;
    }

    printf("【医院收入统计】\n");
    printf("---------------------------\n");
    printf("挂号收入：    %.1f 元\n", reg_income);
    printf("检查收入：    %.1f 元\n", exam_income);
    printf("药品收入：    %.1f 元\n", drug_income);
    printf("住院收入：    %.1f 元\n", inpatient_income);
    printf("---------------------------\n");
    printf("总收入：      %.1f 元\n",
           reg_income + exam_income + drug_income + inpatient_income);

    printf("\n按enter键返回...");
    getchar();
}

// 统计报表 - 二级菜单

void admin_statistics()
{
    while (1)
    {
        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║              统计报表                  ║\n");
        printf("╠════════════════════════════════════════╣\n");
        printf("║  1. 门诊统计                           ║\n");
        printf("║  2. 住院统计                           ║\n");
        printf("║  3. 药品统计                           ║\n");
        printf("║  4. 收入统计                           ║\n");
        printf("║  0. 返回                               ║\n");
        printf("╚════════════════════════════════════════╝\n");
        printf("\n请输入您的选择: ");

        int choice;
        if (scanf("%d", &choice) != 1)
        {
            clear_input();
            continue;
        }
        clear_input();

        if (choice == 0)
            return;

        switch (choice)
        {
        case 1:
            stat_outpatient();
            break;
        case 2:
            stat_inpatient();
            break;
        case 3:
            stat_drug();
            break;
        case 4:
            stat_income();
            break;
        default:
            printf("输入无效，请重新输入。\n");
            printf("按enter键继续...");
            getchar();
            break;
        }
    }
}

void admin_view_inpatients()
{
    while (1)
    {
        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║            患者信息查找                ║\n");
        printf("╚════════════════════════════════════════╝\n\n");

        char keyword[64];
        printf("请输入患者姓名关键字（模糊匹配，输入0返回）: ");
        fgets(keyword, sizeof(keyword), stdin);
        keyword[strcspn(keyword, "\r\n")] = '\0';

        if (strcmp(keyword, "0") == 0)
            return;

        if (strlen(keyword) == 0)
        {
            printf("输入无效，请重新输入。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }

        printf("\n%-8s %-12s %-20s %-8s\n", "病历号", "姓名", "身份证号", "状态");
        printf("----------------------------------------------------\n");

        PatientNode *cur = patient_list;
        int count = 0;
        while (cur != NULL)
        {
            if (strstr(cur->name, keyword) != NULL)
            {
                printf("%-8d %-12s %-20s %s\n",
                       cur->medical_id,
                       cur->name,
                       cur->id_card,
                       cur->is_deleted ? "[已删除]" : "[正常]");
                count++;
            }
            cur = cur->next;
        }

        if (count == 0)
        {
            printf("未找到匹配的患者，请重新检索。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }

        printf("\n共找到 %d 名患者。\n", count);
        printf("\n按enter键继续检索...");
        getchar();
    }
}

// 患者信息管理（逻辑删除）
void admin_manage_patients()
{
    while (1)
    {
        system("cls");
        printf("╔════════════════════════════════════════════════════╗\n");
        printf("║                  修改患者状态                      ║\n");
        printf("╚════════════════════════════════════════════════════╝\n\n");

        int medical_id;
        printf("请输入要修改状态的病历号（输入0返回）: ");
        if (scanf("%d", &medical_id) != 1)
        {
            clear_input();
            printf("输入无效。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }
        clear_input();

        if (medical_id == 0)
            return;

        PatientNode *cur = patient_list;
        int found = 0;
        while (cur != NULL)
        {
            if (cur->medical_id == medical_id)
            {
                found = 1;
                printf("患者：%s（病历号%d），当前状态：%s\n",
                       cur->name, cur->medical_id,
                       cur->is_deleted ? "[已禁用]" : "[正常]");

                if (cur->is_deleted == 0)
                {
                    char msg[120];
                    snprintf(msg, sizeof(msg), "确认禁用患者 %s（病历号%d）？", cur->name, medical_id);
                    if (prompt_yes_no(msg))
                    {
                        cur->is_deleted = 1;
                        save_all();
                        printf("\n患者 %d 已禁用（逻辑删除）。\n", medical_id);
                        printf("注意：数据保留在系统中，仅标记为已禁用，可随时被恢复。\n");
                    }
                    else
                    {
                        printf("已取消。\n");
                    }
                }
                else
                {
                    char msg[120];
                    snprintf(msg, sizeof(msg), "确认恢复患者 %s（病历号%d）的账号？", cur->name, medical_id);
                    if (prompt_yes_no(msg))
                    {
                        cur->is_deleted = 0;
                        save_all();
                        printf("\n患者 %d 已恢复正常状态。\n", medical_id);
                    }
                    else
                    {
                        printf("已取消。\n");
                    }
                }

                printf("按enter键继续...");
                getchar();
                break;
            }
            cur = cur->next;
        }
        if (!found)
        {
            printf("未找到病历号为 %d 的患者。\n", medical_id);
            printf("按enter键继续...");
            getchar();
        }
    }
}

// 管理员主菜单（10秒动态计费）

void admin_menu()
{
    while (1)
    {
        system("cls");
        printf("\n╔══════════════════════════════════════╗\n");
        printf("║  欢迎，%-16s          ║\n", current_user.user_name);
        printf("╠══════════════════════════════════════╣\n");
        printf("║  1. 查找                         ║\n");
        printf("║  2. 管理                         ║\n");
        printf("║  3. 统计报表                     ║\n");
        printf("║  4. 修改密码                     ║\n");
        printf("║  0. 注销登录                     ║\n");
        printf("╚══════════════════════════════════════╝\n");
        printf("\n请输入您的选择: ");

        int choice;
        if (scanf("%d", &choice) != 1)
        {
            clear_input();
            printf("输入无效，请重新输入。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }
        clear_input();

        if (choice == 0)
            break;

        // 查找
        if (choice == 1)
        {
            while (1)
            {
                system("cls");
                printf("\n╔══════════════════════════════════════╗\n");
                printf("║              查找                ║\n");
                printf("╠══════════════════════════════════════╣\n");
                printf("║  1. 科室                         ║\n");
                printf("║  2. 医生                         ║\n");
                printf("║  3. 患者                         ║\n");
                printf("║  0. 返回                         ║\n");
                printf("╚══════════════════════════════════════╝\n");
                printf("\n请输入您的选择: ");

                int c2;
                if (scanf("%d", &c2) != 1)
                {
                    clear_input();
                    continue;
                }
                clear_input();
                if (c2 == 0)
                    break;
                switch (c2)
                {
                case 1:
                    admin_view_departments();
                    break;
                case 2:
                    admin_view_doctors();
                    break;
                case 3:
                    admin_view_inpatients();
                    break;
                default:
                    printf("输入无效，请重新输入。\n");
                    printf("按enter键继续...");
                    getchar();
                    break;
                }
            }
            continue;
        }

        // 管理
        if (choice == 2)
        {
            while (1)
            {
                system("cls");
                printf("\n╔══════════════════════════════════════╗\n");
                printf("║              管理                ║\n");
                printf("╠══════════════════════════════════════╣\n");
                printf("║  1. 患者                         ║\n");
                printf("║  2. 员工                         ║\n");
                printf("║  3. 药品                         ║\n");
                printf("║  4. 科室                         ║\n");
                printf("║  5. 病房                         ║\n");
                printf("║  0. 返回                         ║\n");
                printf("╚══════════════════════════════════════╝\n");
                printf("\n请输入您的选择: ");

                int c2;
                if (scanf("%d", &c2) != 1)
                {
                    clear_input();
                    continue;
                }
                clear_input();
                if (c2 == 0)
                    break;
                if (c2 == 1)
                {
                    admin_manage_patients();
                    continue;
                }
                if (c2 == 2)
                {
                    admin_manage_staff();
                    continue;
                }
                if (c2 == 3)
                {
                    admin_manage_drugs();
                    continue;
                }
                if (c2 == 4)
                {
                    admin_manage_departments();
                    continue;
                }

                // 5. 病房 —— 为指定病房加床位
                if (c2 == 5)
                {
                    while (1)
                    {
                        system("cls");
                        printf("╔════════════════════════════════════════╗\n");
                        printf("║              病房管理                  ║\n");
                        printf("╚════════════════════════════════════════╝\n\n");

                        // 列出所有病房
                        printf("%-8s %-10s %-12s %-8s %-8s\n",
                               "病房ID", "科室", "类型", "总床位", "已占用");
                        printf("--------------------------------------------------\n");
                        RoomNode *r = room_list;
                        int has_room = 0;
                        while (r != NULL)
                        {
                            const char *type_name = "普通多人间";
                            if (r->room_type == ROOM_B) type_name = "术后观察";
                            if (r->room_type == ROOM_C) type_name = "急诊留观";
                            printf("%-8d %-10s %-12s %-8d %-8d\n",
                                   r->room_id,
                                   get_dept_name(r->dept_id),
                                   type_name,
                                   r->capacity,
                                   r->current);
                            has_room = 1;
                            r = r->next;
                        }
                        printf("--------------------------------------------------\n");
                        if (!has_room)
                            printf("（暂无病房记录）\n");

                        printf("\n请输入要增加床位的病房ID（输入0返回）: ");
                        int room_id;
                        if (scanf("%d", &room_id) != 1)
                        {
                            clear_input();
                            printf("输入无效，请重新输入。\n");
                            printf("按enter键继续...");
                            getchar();
                            continue;
                        }
                        clear_input();
                        if (room_id == 0)
                            break;

                        // 查找病房
                        RoomNode *target = room_list;
                        while (target != NULL && target->room_id != room_id)
                            target = target->next;

                        if (target == NULL)
                        {
                            printf("未找到病房ID为 %d 的病房。\n", room_id);
                            printf("按enter键继续...");
                            getchar();
                            continue;
                        }

                        printf("当前病房 %d 总床位：%d，已占用：%d。\n",
                               target->room_id, target->capacity, target->current);
                        printf("请输入要增加的床位数量: ");
                        int add_beds;
                        if (scanf("%d", &add_beds) != 1 || add_beds <= 0)
                        {
                            clear_input();
                            printf("输入无效，床位数量须为正整数。\n");
                            printf("按enter键继续...");
                            getchar();
                            continue;
                        }
                        clear_input();

                        char msg[100];
                        snprintf(msg, sizeof(msg),
                                 "确认为病房 %d 增加 %d 个床位？", room_id, add_beds);
                        if (prompt_yes_no(msg))
                        {
                            target->capacity += add_beds;
                            save_all();
                            printf("已成功为病房 %d 增加 %d 个床位，当前总床位：%d。\n",
                                   room_id, add_beds, target->capacity);
                        }
                        else
                        {
                            printf("已取消。\n");
                        }
                        printf("按enter键继续...");
                        getchar();
                    }
                    continue;
                }

                printf("输入无效，请重新输入。\n");
                printf("按enter键继续...");
                getchar();
            }
            continue;
        }

        // 统计报表
        if (choice == 3)
        {
            admin_statistics();
            continue;
        }

        // 修改密码
        if (choice == 4)
        {
            change_password();
            continue;
        }

        printf("输入无效，请重新输入。\n");
        printf("按enter键继续...");
        getchar();
    }
}