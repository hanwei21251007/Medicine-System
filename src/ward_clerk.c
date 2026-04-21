// ward_clerk.c
// 病房员工（病区文员）功能模块
// 负责：审核住院申请、住院登记、出院登记、查看在院患者列表

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/structs.h"

// 外部链表和函数声明
extern StaffNode *staff_list;
extern PatientNode *patient_list;
extern DepartmentNode *dept_list;
extern RoomNode *room_list;
extern InpatientNode *inpatient_list;
extern InpatientApplyNode *inpatient_apply_list;
extern CurrentUser current_user;

extern void save_all();
extern int generate_inpatient_id();
extern const void clear_input();
extern int prompt_yes_no(const char *prompt);
extern const char *get_title_name(DoctorTitle title);
extern float calc_current_fee(InpatientNode *ip);
extern void change_password();

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

// 根据医生ID查医生姓名
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

// 根据病房ID查每周费用
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

// ──────────────────────────────────────────
// 1. 审核住院申请
// ──────────────────────────────────────────

void ward_review_apply()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║          住院申请审核                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    printf("%-6s %-10s %-10s %-16s %-22s %-8s\n",
           "申请ID", "病历号", "申请医生", "科室", "申请时间", "状态");
    printf("--------------------------------------------------------------------------\n");

    InpatientApplyNode *cur = inpatient_apply_list;
    int count = 0;
    while (cur != NULL)
    {
        char *status_str = "待审核";
        if (cur->is_approved == 1)
            status_str = "已批准";
        if (cur->is_approved == 2)
            status_str = "已拒绝";
        printf("%-6d %-10d %-10s %-16s %-22s %-8s\n",
               cur->apply_id, cur->patient_id,
               get_doctor_name(cur->doctor_id),
               get_dept_name(cur->dept_id),
               cur->apply_time, status_str);
        count++;
        cur = cur->next;
    }
    if (count == 0)
    {
        printf("暂无住院申请。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    // 查找待审核申请
    InpatientApplyNode *target = NULL;
    while (target == NULL)
    {
        printf("\n输入要审核的申请ID (0=返回): ");
        int apply_id;
        if (scanf("%d", &apply_id) != 1)
        {
            clear_input();
            printf("输入无效。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            continue;
        }
        clear_input();
        if (apply_id == 0)
            return;

        InpatientApplyNode *p = inpatient_apply_list;
        while (p != NULL)
        {
            if (p->apply_id == apply_id)
            {
                target = p;
                break;
            }
            p = p->next;
        }
        if (target == NULL)
        {
            printf("未找到该申请。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
        }
    }

    if (target->is_approved != 0)
    {
        printf("该申请已处理过，无需重复操作。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    printf("病历号：%d，申请医生：%s，科室：%s\n",
           target->patient_id,
           get_doctor_name(target->doctor_id),
           get_dept_name(target->dept_id));
    printf("1. 批准  2. 拒绝  0. 取消\n");
    printf("请选择: ");

    int decision = -1;
    while (decision < 0)
    {
        if (scanf("%d", &decision) != 1 ||
            (decision != 0 && decision != 1 && decision != 2))
        {
            clear_input();
            printf("输入无效。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            decision = -1;
            continue;
        }
        clear_input();
    }

    if (decision == 0)
    {
        printf("已取消。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    if (decision == 2)
    {
        target->is_approved = 2;
        save_all();
        printf("已拒绝该住院申请。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    // 批准前检查患者是否已在院
    InpatientNode *ip_check = inpatient_list;
    while (ip_check != NULL)
    {
        if (ip_check->patient_id == target->patient_id &&
            ip_check->is_discharged == 0)
        {
            printf("该患者当前已在院，无法重复批准住院申请。\n");
            printf("按enter键返回...");
            getchar();
            return;
        }
        ip_check = ip_check->next;
    }

    // 批准：自动完成住院登记，分配床位
    target->is_approved = 1;

    RoomNode *best_room = NULL;
    RoomNode *room = room_list;
    while (room != NULL)
    {
        if (room->dept_id == target->dept_id &&
            room->current < room->capacity)
        {
            if (best_room == NULL)
                best_room = room;
            else if (room->current > best_room->current)
                best_room = room;
        }
        room = room->next;
    }

    if (best_room == NULL)
    {
        printf("该科室暂无空余床位，申请已批准但暂缓入院。\n");
        save_all();
        printf("按enter键返回...");
        getchar();
        return;
    }

    InpatientNode *node = (InpatientNode *)malloc(sizeof(InpatientNode));
    if (node == NULL)
    {
        printf("内存分配失败。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    node->inpatient_id = generate_inpatient_id();
    node->patient_id = target->patient_id;
    node->doctor_id = target->doctor_id;
    node->dept_id = target->dept_id;
    node->room_id = best_room->room_id;
    node->bed_num = best_room->current + 1;
    node->days = 0;
    node->total_fee = 0.0f;
    node->is_discharged = 0;
    get_today(node->admit_date);
    strcpy(node->discharge_date, "-");
    node->next = NULL;

    if (inpatient_list == NULL)
    {
        inpatient_list = node;
    }
    else
    {
        InpatientNode *tail = inpatient_list;
        while (tail->next != NULL)
            tail = tail->next;
        tail->next = node;
    }

    best_room->current += 1;
    save_all();

    printf("批准成功！已自动完成住院登记。\n");
    printf("住院ID：%d，病房：%d号，床位：%d号\n",
           node->inpatient_id, node->room_id, node->bed_num);
    printf("按enter键返回...");
    getchar();
}

// ──────────────────────────────────────────
// 2. 住院登记（直接登记，无需申请）
// ──────────────────────────────────────────

void ward_admit_patient()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              住院登记                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 查找患者
    PatientNode *patient = NULL;
    int medical_id = 0;
    while (patient == NULL)
    {
        printf("输入患者病历号: ");
        if (scanf("%d", &medical_id) != 1)
        {
            clear_input();
            printf("输入无效。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            continue;
        }
        clear_input();

        int denied = 0;
        PatientNode *p = patient_list;
        while (p != NULL)
        {
            if (p->medical_id == medical_id)
            {
                if (p->is_deleted == 1)
                {
                    printf("该患者已被禁用，无法办理住院登记。\n");
                    if (!prompt_yes_no("是否重试？"))
                        return;
                    denied = 1;
                    break;
                }
                patient = p;
                break;
            }
            p = p->next;
        }
        if (!denied && patient == NULL)
        {
            printf("未找到该病历号对应的患者。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
        }
    }

    // 检查是否已在院
    InpatientNode *check = inpatient_list;
    while (check != NULL)
    {
        if (check->patient_id == medical_id && check->is_discharged == 0)
        {
            printf("该患者当前已在院，无需重复登记。\n");
            printf("按enter键返回...");
            getchar();
            return;
        }
        check = check->next;
    }

    printf("患者姓名：%s\n\n", patient->name);

    // 查找主治医生
    StaffNode *doctor = NULL;
    int doctor_id = 0;
    while (doctor == NULL)
    {
        printf("输入主治医生工号: ");
        if (scanf("%d", &doctor_id) != 1)
        {
            clear_input();
            printf("输入无效。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            continue;
        }
        clear_input();

        StaffNode *s = staff_list;
        while (s != NULL)
        {
            if (s->id == doctor_id && s->role == ROLE_DOCTOR)
            {
                doctor = s;
                break;
            }
            s = s->next;
        }
        if (doctor == NULL)
        {
            printf("未找到该工号对应的医生。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
        }
    }

    int dept_id = doctor->dept_id;
    printf("科室：%s\n\n", get_dept_name(dept_id));

    // 分配床位
    RoomNode *best_room = NULL;
    RoomNode *cur = room_list;
    while (cur != NULL)
    {
        if (cur->dept_id == dept_id && cur->current < cur->capacity)
        {
            if (best_room == NULL)
                best_room = cur;
            else if (cur->current > best_room->current)
                best_room = cur;
        }
        cur = cur->next;
    }

    if (best_room == NULL)
    {
        printf("该科室暂无空余床位，无法办理住院。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    int bed_num = best_room->current + 1;
    char *type_name = "普通多人间";
    if (best_room->room_type == ROOM_B)
        type_name = "术后观察";
    if (best_room->room_type == ROOM_C)
        type_name = "急症留观";

    printf("分配病房：%d号房（%s），床位：%d号，计费单位：%.1f元/10秒\n\n",
           best_room->room_id, type_name, bed_num, best_room->daily_fee);

    if (!prompt_yes_no("确认办理住院？"))
    {
        printf("已取消。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    InpatientNode *node = (InpatientNode *)malloc(sizeof(InpatientNode));
    if (node == NULL)
    {
        printf("内存分配失败，住院登记取消。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    node->inpatient_id = generate_inpatient_id();
    node->patient_id = medical_id;
    node->doctor_id = doctor_id;
    node->dept_id = dept_id;
    node->room_id = best_room->room_id;
    node->bed_num = bed_num;
    node->days = 0;
    node->total_fee = 0.0f;
    node->is_discharged = 0;
    get_today(node->admit_date);
    strcpy(node->discharge_date, "-");
    node->next = NULL;

    if (inpatient_list == NULL)
    {
        inpatient_list = node;
    }
    else
    {
        InpatientNode *tail = inpatient_list;
        while (tail->next != NULL)
            tail = tail->next;
        tail->next = node;
    }

    best_room->current += 1;
    save_all();

    printf("\n住院登记成功！住院ID：%d，入院时间：%s\n",
           node->inpatient_id, node->admit_date);
    printf("按enter键返回...");
    getchar();
}

// ──────────────────────────────────────────
// 3. 出院登记
// ──────────────────────────────────────────

void ward_discharge_patient()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              出院登记                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    InpatientNode *target = NULL;
    while (target == NULL)
    {
        printf("输入患者病历号: ");
        int medical_id;
        if (scanf("%d", &medical_id) != 1)
        {
            clear_input();
            printf("输入无效。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            continue;
        }
        clear_input();

        InpatientNode *p = inpatient_list;
        while (p != NULL)
        {
            if (p->patient_id == medical_id && p->is_discharged == 0)
            {
                target = p;
                break;
            }
            p = p->next;
        }
        if (target == NULL)
        {
            printf("未找到该患者的在院记录。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
        }
    }

    if (target->is_discharged == 1)
    {
        printf("该患者已办理出院，无需重复操作。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    char today[25];
    get_today(today);

    printf("患者：%s\n", get_patient_name(target->patient_id));
    printf("入院时间：%s\n", target->admit_date);
    printf("出院时间：%s\n", today);
    printf("住院累计费用：%.1f 元\n\n", calc_current_fee(target));

    if (!prompt_yes_no("确认办理出院？"))
    {
        printf("已取消。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    // 固化费用和周期数
    {
        struct tm tm_admit = {0};
        sscanf(target->admit_date, "%d-%d-%d %d:%d:%d",
               &tm_admit.tm_year, &tm_admit.tm_mon, &tm_admit.tm_mday,
               &tm_admit.tm_hour, &tm_admit.tm_min, &tm_admit.tm_sec);
        tm_admit.tm_year -= 1900;
        tm_admit.tm_mon -= 1;
        tm_admit.tm_isdst = -1;
        time_t t_admit = mktime(&tm_admit);
        time_t t_now = time(NULL);
        int p = (int)(difftime(t_now, t_admit) / 10.0) + 1;
        if (p < 1)
            p = 1;
        target->days = p;
        target->total_fee = get_room_fee(target->room_id) * (float)p;
    }
    strcpy(target->discharge_date, today);
    target->is_discharged = 1;

    RoomNode *room = room_list;
    while (room != NULL)
    {
        if (room->room_id == target->room_id)
        {
            if (room->current > 0)
                room->current -= 1;
            break;
        }
        room = room->next;
    }

    save_all();

    printf("\n出院登记成功！结算费用：%.1f 元\n", target->total_fee);
    printf("按enter键返回...");
    getchar();
}

// ──────────────────────────────────────────
// 4. 查看在院患者列表
// ──────────────────────────────────────────

void ward_view_inpatients()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            在院患者列表                ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    printf("%-8s %-10s %-16s %-6s %-6s %-22s %-10s\n",
           "住院ID", "姓名", "科室", "病房", "床位", "入院时间", "已累计费用");
    printf("------------------------------------------------------------------------------\n");

    InpatientNode *cur = inpatient_list;
    int count = 0;
    while (cur != NULL)
    {
        if (cur->is_discharged == 0)
        {
            printf("%-8d %-10s %-16s %-6d %-6d %-22s %.1f元\n",
                   cur->inpatient_id,
                   get_patient_name(cur->patient_id),
                   get_dept_name(cur->dept_id),
                   cur->room_id,
                   cur->bed_num,
                   cur->admit_date,
                   calc_current_fee(cur));
            count++;
        }
        cur = cur->next;
    }

    if (count == 0)
        printf("当前无在院患者。\n");

    printf("\n按enter键返回...");
    getchar();
}

// ──────────────────────────────────────────
// 病房员工主菜单
// ──────────────────────────────────────────

void ward_clerk_menu()
{
    while (1)
    {
        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║  欢迎，%-16s              ║\n", current_user.user_name);
        printf("╠════════════════════════════════════════╣\n");
        printf("║  1. 审核住院申请                       ║\n");
        printf("║  2. 住院登记                           ║\n");
        printf("║  3. 出院登记                           ║\n");
        printf("║  4. 查看在院患者                       ║\n");
        printf("║  5. 修改密码                           ║\n");
        printf("║  0. 注销登录                           ║\n");
        printf("╚════════════════════════════════════════╝\n");
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

        if (choice < 1 || choice > 5)
        {
            printf("输入无效，请重新输入。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }

        switch (choice)
        {
        case 1:
            ward_review_apply();
            break;
        case 2:
            ward_admit_patient();
            break;
        case 3:
            ward_discharge_patient();
            break;
        case 4:
            ward_view_inpatients();
            break;
        case 5:
            change_password();
            break;
        }
    }
}