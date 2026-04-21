//何星彤组员负责
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/structs.h"


// 外部链表和函数声明
extern StaffNode *staff_list;
extern PatientNode *patient_list;
extern DepartmentNode *dept_list;
extern ExamItemNode *exam_item_list;
extern DrugNode *drug_list;
extern RoomNode *room_list;
extern RegistrationNode *reg_list;
extern MedicalRecordNode *record_list;
extern ExamOrderNode *exam_order_list;
extern PrescriptionNode *prescription_list;
extern InpatientNode *inpatient_list;
extern CurrentUser current_user;

extern void save_all();
extern int generate_reg_id();
extern int get_queue_count(int dept_id);
extern float get_title_fee(DoctorTitle title);
extern const char *get_title_name(DoctorTitle title);
extern const void clear_input();
extern void change_password();

// 获取当前时间字符串
static void get_now(char *buf)
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

// 根据医生工号查医生姓名
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

// 根据药品ID查药品名
static const char *get_drug_name(int drug_id)
{
    DrugNode *cur = drug_list;
    while (cur != NULL)
    {
        if (cur->drug_id == drug_id)
            return cur->drug_name;
        cur = cur->next;
    }
    return "未知药品";
}

// 根据检查ID查检查项目名
static const char *get_exam_name(int exam_id)
{
    ExamItemNode *cur = exam_item_list;
    while (cur != NULL)
    {
        if (cur->exam_id == exam_id)
            return cur->exam_name;
        cur = cur->next;
    }
    return "未知检查";
}

// 挂号状态转文字
static const char *get_reg_status_name(RegistrationNode *reg)
{
    if (reg->is_cancelled)
        return "已作废";
    switch (reg->status)
    {
    case STATUS_PENDING_PAY:
        return "待接诊";
    case STATUS_PENDING_DO:
        return "已接诊";
    case STATUS_DONE:
        return "已完成";
    default:
        return "未知";
    }
}

// 处方/检查状态转文字
static const char *get_item_status_name(ItemStatus status)
{
    switch (status)
    {
    case STATUS_PENDING_PAY:
        return "待缴费";
    case STATUS_PENDING_DO:
        return "已缴费待执行";
    case STATUS_DONE:
        return "已完成";
    case STATUS_CANCELLED:
        return "已取消";
    default:
        return "未知";
    }
}

// 门诊挂号

void patient_register_outpatient()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              门诊挂号                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 显示科室列表
    printf("【科室列表】\n");
    printf("%-6s %-16s %-10s\n", "科室ID", "科室名称", "基础挂号费");
    printf("----------------------------------\n");
    DepartmentNode *dept = dept_list;
    while (dept != NULL)
    {
        if (dept->is_deleted == 0)   // 只显示正常科室，停用科室不出现在列表
            printf("%-6d %-16s %.1f元\n",
                   dept->dept_id, dept->dept_name, dept->base_reg_fee);
        dept = dept->next;
    }

    // 选择科室
    DepartmentNode *chosen_dept = NULL;
    while (chosen_dept == NULL)
    {
        printf("\n请选择科室ID: ");
        int dept_id;
        if (scanf("%d", &dept_id) != 1)
        {
            clear_input();
            printf("输入无效。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            continue;
        }
        clear_input();

        DepartmentNode *d = dept_list;
        while (d != NULL)
        {
            if (d->dept_id == dept_id)
            {
                if (d->is_deleted == 1)
                {
                    printf("该科室已停用，无法挂号，请选择其他科室。\n");
                    if (!prompt_yes_no("是否重新选择？"))
                        return;
                }
                else
                {
                    chosen_dept = d;
                }
                break;
            }
            d = d->next;
        }
        if (chosen_dept == NULL && d == NULL)
        {
            printf("未找到该科室。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
        }
    }

    // 检查该患者在该科室是否已有未完成的挂号
    RegistrationNode *chk = reg_list;
    while (chk != NULL)
    {
        if (chk->patient_id == current_user.user_id &&
            chk->dept_id    == chosen_dept->dept_id  &&
            chk->is_cancelled == 0                   &&
            chk->status != STATUS_DONE)
        {
            printf("\n您在【%s】已有一条未完成的挂号（挂号ID：%d，状态：%s）。\n",
                   chosen_dept->dept_name,
                   chk->reg_id,
                   get_reg_status_name(chk));
            printf("同一科室只能同时挂1次，请等待本次就诊完成或号码作废后再挂号。\n");
            printf("\n按enter键返回...");
            getchar();
            return;
        }
        chk = chk->next;
    }

    // 显示该科室医生列表
    printf("\n【%s 医生列表】\n", chosen_dept->dept_name);
    printf("%-8s %-10s %-12s %-10s\n", "工号", "姓名", "职称", "挂号费");
    printf("------------------------------------------\n");

    StaffNode *s = staff_list;
    int doc_count = 0;
    while (s != NULL)
    {
        if (s->role == ROLE_DOCTOR && s->dept_id == chosen_dept->dept_id && s->is_deleted == 0)
        {
            float fee = chosen_dept->base_reg_fee + get_title_fee(s->title);
            printf("%-8d %-10s %-12s %.1f元\n",
                   s->id, s->name, get_title_name(s->title), fee);
            doc_count++;
        }
        s = s->next;
    }

    if (doc_count == 0)
    {
        printf("该科室暂无医生。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    // 选择医生
    StaffNode *chosen_doctor = NULL;
    while (chosen_doctor == NULL)
    {
        printf("\n请选择医生工号: ");
        int doc_id;
        if (scanf("%d", &doc_id) != 1)
        {
            clear_input();
            printf("输入无效。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            continue;
        }
        clear_input();

        StaffNode *d = staff_list;
        while (d != NULL)
        {
            if (d->id == doc_id && d->role == ROLE_DOCTOR &&
                d->dept_id == chosen_dept->dept_id && d->is_deleted == 0)
            {
                chosen_doctor = d;
                break;
            }
            d = d->next;
        }
        if (chosen_doctor == NULL)
        {
            printf("未找到该医生，请从列表中选择。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
        }
    }

    // 计算挂号费和排队号
    float reg_fee = chosen_dept->base_reg_fee + get_title_fee(chosen_doctor->title);
    int queue_num = get_queue_count(chosen_dept->dept_id) + 1;

    // 创建挂号记录
    RegistrationNode *node = (RegistrationNode *)malloc(sizeof(RegistrationNode));
    if (node == NULL)
    {
        printf("内存分配失败，挂号取消。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    node->reg_id = generate_reg_id();
    node->patient_id = current_user.user_id;
    node->doctor_id = chosen_doctor->id;
    node->dept_id = chosen_dept->dept_id;
    node->reg_fee = reg_fee;
    node->queue_num = queue_num;
    node->status = STATUS_PENDING_PAY;
    node->is_cancelled = 0;
    strcpy(node->cancel_time, "-");
    get_now(node->date);
    node->next = NULL;

    if (reg_list == NULL)
    {
        reg_list = node;
    }
    else
    {
        RegistrationNode *tail = reg_list;
        while (tail->next != NULL)
            tail = tail->next;
        tail->next = node;
    }

    save_all();

    printf("\n挂号成功！\n");
    printf("  挂号ID：%d\n", node->reg_id);
    printf("  科室：%s\n", chosen_dept->dept_name);
    printf("  医生：%s\n", chosen_doctor->name);
    printf("  排队号：%d\n", queue_num);
    printf("  挂号费：%.1f元\n", reg_fee);
    printf("  挂号时间：%s\n", node->date);
    printf("  请在60秒内到诊室等候，超时号码将自动作废。\n");
    printf("\n按enter键返回...");
    getchar();
}

// 查看就诊记录（挂号+病历）

void patient_view_records()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            就诊记录                    ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 挂号记录
    printf("【挂号记录】\n");
    printf("%-6s %-16s %-10s %-6s %-22s %-8s\n",
           "挂号ID", "科室", "医生", "排队号", "挂号时间", "状态");
    printf("------------------------------------------------------------------------\n");

    RegistrationNode *reg = reg_list;
    int reg_count = 0;
    while (reg != NULL)
    {
        if (reg->patient_id == current_user.user_id)
        {
            printf("%-6d %-16s %-10s %-6d %-22s %-8s\n",
                   reg->reg_id,
                   get_dept_name(reg->dept_id),
                   get_doctor_name(reg->doctor_id),
                   reg->queue_num,
                   reg->date,
                   get_reg_status_name(reg));
            reg_count++;
        }
        reg = reg->next;
    }
    if (reg_count == 0)
        printf("暂无挂号记录。\n");

    // 病历记录
    printf("\n【病历记录】\n");
    printf("%-6s %-16s %-10s %-22s %-20s\n",
           "病历ID", "科室", "医生", "就诊时间", "诊断");
    printf("------------------------------------------------------------------------\n");

    MedicalRecordNode *rec = record_list;
    int rec_count = 0;
    while (rec != NULL)
    {
        if (rec->patient_id == current_user.user_id)
        {
            printf("%-6d %-16s %-10s %-22s %-20s\n",
                   rec->record_id,
                   get_dept_name(rec->dept_id),
                   get_doctor_name(rec->doctor_id),
                   rec->date,
                   rec->diagnosis);
            rec_count++;
        }
        rec = rec->next;
    }
    if (rec_count == 0)
        printf("暂无病历记录。\n");

    printf("\n按enter键返回...");
    getchar();
}

// 查看检查记录

void patient_view_exams()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            检查记录                    ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    printf("%-6s %-16s %-8s %-16s\n",
           "申请ID", "检查项目", "费用", "状态");
    printf("----------------------------------------------\n");

    ExamOrderNode *cur = exam_order_list;
    int count = 0;
    while (cur != NULL)
    {
        if (cur->patient_id == current_user.user_id)
        {
            char *result_str = "-";
            if (cur->status == STATUS_DONE)
            {
                if (cur->result == EXAM_RESULT_NORMAL)
                    result_str = "正常";
                if (cur->result == EXAM_RESULT_DRUG)
                    result_str = "需开药";
                if (cur->result == EXAM_RESULT_INPATIENT)
                    result_str = "需住院";
            }
            printf("%-6d %-16s %-8.1f %-12s 结果：%s\n",
                   cur->order_id,
                   get_exam_name(cur->exam_id),
                   cur->price,
                   get_item_status_name(cur->status),
                   result_str);
            count++;
        }
        cur = cur->next;
    }
    if (count == 0)
    {
        printf("暂无检查记录。\n");
        printf("\n按enter键返回...");
        getchar();
        return;
    }

    // 缴费/取消操作
    printf("\n请选择操作：\n");
    printf("  1. 为某项检查缴费\n");
    printf("  2. 取消某项检查账单\n");
    printf("  0. 暂不操作\n");
    printf("请输入您的选择: ");

    int op;
    if (scanf("%d", &op) != 1) { clear_input(); op = 0; }
    clear_input();

    if (op == 1 || op == 2)
    {
        ExamOrderNode *target = NULL;
        while (target == NULL)
        {
            printf(op == 1 ? "输入要缴费的申请ID: " : "输入要取消的申请ID: ");
            int order_id;
            if (scanf("%d", &order_id) != 1)
            {
                clear_input();
                printf("输入无效。\n");
                if (!prompt_yes_no("是否重试？"))
                    break;
                continue;
            }
            clear_input();

            ExamOrderNode *p = exam_order_list;
            while (p != NULL)
            {
                if (p->order_id == order_id &&
                    p->patient_id == current_user.user_id &&
                    p->status == STATUS_PENDING_PAY)
                {
                    target = p;
                    break;
                }
                p = p->next;
            }
            if (target == NULL)
            {
                printf("未找到该检查申请，或该申请不在待缴费状态。\n");
                if (!prompt_yes_no("是否重试？"))
                    break;
            }
        }

        if (target != NULL)
        {
            if (op == 1)
            {
                char msg[100];
                snprintf(msg, sizeof(msg), "检查费用：%.1f元，确认缴费？", target->price);
                if (prompt_yes_no(msg))
                {
                    target->status = STATUS_PENDING_DO;
                    save_all();
                    printf("缴费成功！医生可为您填写检查结果。\n");
                }
                else
                {
                    printf("已取消操作。\n");
                }
            }
            else
            {
                char msg[100];
                snprintf(msg, sizeof(msg), "确认取消该检查账单（%.1f元）？取消后无法恢复。", target->price);
                if (prompt_yes_no(msg))
                {
                    target->status = STATUS_CANCELLED;
                    save_all();
                    printf("账单已取消。\n");
                }
                else
                {
                    printf("已取消操作。\n");
                }
            }
        }
    }

    printf("\n按enter键返回...");
    getchar();
}

// 查看处方记录

void patient_view_prescriptions()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            处方记录                    ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    printf("%-6s %-20s %-6s %-8s %-14s\n",
           "处方ID", "药品名称", "数量", "总价", "状态");
    printf("----------------------------------------------------\n");

    PrescriptionNode *cur = prescription_list;
    int count = 0;
    while (cur != NULL)
    {
        if (cur->patient_id == current_user.user_id)
        {
            printf("%-6d %-20s %-6d %-8.1f %-14s\n",
                   cur->prescription_id,
                   get_drug_name(cur->drug_id),
                   cur->quantity,
                   cur->price,
                   get_item_status_name(cur->status));
            count++;
        }
        cur = cur->next;
    }
    if (count == 0)
    {
        printf("暂无处方记录。\n");
        printf("\n按enter键返回...");
        getchar();
        return;
    }

    // 缴费/取消操作
    printf("\n请选择操作：\n");
    printf("  1. 为某条处方缴费\n");
    printf("  2. 取消某条处方账单\n");
    printf("  0. 暂不操作\n");
    printf("请输入您的选择: ");

    int op;
    if (scanf("%d", &op) != 1) { clear_input(); op = 0; }
    clear_input();

    if (op == 1 || op == 2)
    {
        PrescriptionNode *target = NULL;
        while (target == NULL)
        {
            printf(op == 1 ? "输入要缴费的处方ID: " : "输入要取消的处方ID: ");
            int presc_id;
            if (scanf("%d", &presc_id) != 1)
            {
                clear_input();
                printf("输入无效。\n");
                if (!prompt_yes_no("是否重试？"))
                    break;
                continue;
            }
            clear_input();

            PrescriptionNode *p = prescription_list;
            while (p != NULL)
            {
                if (p->prescription_id == presc_id &&
                    p->patient_id == current_user.user_id &&
                    p->status == STATUS_PENDING_PAY)
                {
                    target = p;
                    break;
                }
                p = p->next;
            }
            if (target == NULL)
            {
                printf("未找到该处方，或该处方不在待缴费状态。\n");
                if (!prompt_yes_no("是否重试？"))
                    break;
            }
        }

        if (target != NULL)
        {
            if (op == 1)
            {
                char msg[100];
                snprintf(msg, sizeof(msg), "处方费用：%.1f元，确认缴费？", target->price);
                if (prompt_yes_no(msg))
                {
                    target->status = STATUS_PENDING_DO;
                    save_all();
                    printf("缴费成功！药剂师可为您发药。\n");
                }
                else
                {
                    printf("已取消操作。\n");
                }
            }
            else
            {
                char msg[100];
                snprintf(msg, sizeof(msg), "确认取消该处方账单（%.1f元）？取消后无法恢复。", target->price);
                if (prompt_yes_no(msg))
                {
                    target->status = STATUS_CANCELLED;
                    save_all();
                    printf("账单已取消。\n");
                }
                else
                {
                    printf("已取消操作。\n");
                }
            }
        }
    }

    printf("\n按enter键返回...");
    getchar();
}

// 查看住院信息

void patient_view_inpatient()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            住院信息                    ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    InpatientNode *cur = inpatient_list;
    int count = 0;
    while (cur != NULL)
    {
        if (cur->patient_id == current_user.user_id)
        {
            char *status_str = cur->is_discharged ? "已出院" : "在院中";
            printf("住院ID：%d\n", cur->inpatient_id);
            printf("科室：%s\n", get_dept_name(cur->dept_id));
            printf("病房：%d号房\n", cur->room_id);
            printf("床位：%d号\n", cur->bed_num);
            printf("入院时间：%s\n", cur->admit_date);
            if (cur->is_discharged)
            {
                printf("出院时间：%s\n", cur->discharge_date);
                printf("住院费用：%.1f元\n", cur->total_fee);
            }
            else
            {
                printf("当前累计费用：%.1f元\n", calc_current_fee(cur));
            }
            printf("状态：%s\n", status_str);
            printf("----------------------------\n");
            count++;
        }
        cur = cur->next;
    }
    if (count == 0)
        printf("暂无住院记录。\n");

    printf("\n按enter键返回...");
    getchar();
}

// 患者主菜单

void patient_menu()
{
    while (1)
    {
        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║  欢迎，%-16s                            ║\n", current_user.user_name);
        printf("║  病历号：%-6d                           ║\n", current_user.user_id);
        printf("╠════════════════════════════════════════╣\n");
        printf("║  1. 门诊挂号                           ║\n");
        printf("║  2. 就诊记录                           ║\n");
        printf("║  3. 检查记录                           ║\n");
        printf("║  4. 处方记录                           ║\n");
        printf("║  5. 住院信息                           ║\n");
        printf("║  6. 修改密码                           ║\n");
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

        if (choice < 1 || choice > 6)
        {
            printf("输入无效，请重新输入。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }

        switch (choice)
        {
        case 1:
            patient_register_outpatient();
            break;
        case 2:
            patient_view_records();
            break;
        case 3:
            patient_view_exams();
            break;
        case 4:
            patient_view_prescriptions();
            break;
        case 5:
            patient_view_inpatient();
            break;
        case 6:
            change_password();
            break;
        }
    }
}