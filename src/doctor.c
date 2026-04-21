//韩丹组员负责
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
extern RegistrationNode *reg_list;
extern MedicalRecordNode *record_list;
extern ExamOrderNode *exam_order_list;
extern PrescriptionNode *prescription_list;
extern InpatientApplyNode *inpatient_apply_list;
extern CurrentUser current_user;

// 外部函数
extern void save_all();
extern void clear_input();
extern int generate_exam_order_id();
extern int generate_prescription_id();
extern int generate_apply_id();
extern int generate_record_id();
extern void change_password();

// 前向声明
static void doctor_prescribe(int record_id, int patient_id);
static void doctor_apply_inpatient(int patient_id);

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

// 把 "YYYY-MM-DD HH:MM:SS" 格式的字符串转成 time_t
static time_t str_to_time(const char *str)
{
    struct tm t = {0};
    sscanf(str, "%d-%d-%d %d:%d:%d",
           &t.tm_year, &t.tm_mon, &t.tm_mday,
           &t.tm_hour, &t.tm_min, &t.tm_sec);
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    return mktime(&t);
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

// 超时作废检查（每次刷新候诊列表时触发）
// 超过60秒未接诊的号自动作废

static void check_timeout()
{
    time_t now = time(NULL);
    int updated = 0;

    RegistrationNode *cur = reg_list;
    while (cur != NULL)
    {
        if (cur->status == STATUS_PENDING_PAY && cur->is_cancelled == 0)
        {
            time_t reg_time = str_to_time(cur->date);
            if (now - reg_time > 60)
            {
                cur->is_cancelled = 1;
                get_now(cur->cancel_time);
                updated = 1;
                printf("[系统] 挂号ID %d（病历号%d）已超时作废。\n",
                       cur->reg_id, cur->patient_id);
            }
        }
        cur = cur->next;
    }

    if (updated)
    {
        save_all();
        printf("按enter键继续...");
        getchar();
    }
}

// 开检查申请

static void doctor_order_exam(int record_id, int patient_id)
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            开具检查申请                ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 显示本科室检查项目
    printf("DEBUG: dept_id = %d\n", current_user.dept_id);
    printf("【%s 检查项目】\n", get_dept_name(current_user.dept_id));
    printf("%-6s %-20s %-8s\n", "项目ID", "项目名称", "费用");
    printf("----------------------------------\n");

    ExamItemNode *item = exam_item_list;
    int item_count = 0;
    while (item != NULL)
    {
        if (item->dept_id == current_user.dept_id)
        {
            printf("%-6d %-20s %.1f元\n",
                   item->exam_id, item->exam_name, item->price);
            item_count++;
        }
        item = item->next;
    }

    if (item_count == 0)
    {
        printf("本科室暂无检查项目。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    // 选择检查项目
    ExamItemNode *chosen = NULL;
    while (chosen == NULL)
    {
        printf("\n请选择检查项目ID (0=取消): ");
        int exam_id;
        if (scanf("%d", &exam_id) != 1)
        {
            clear_input();
            printf("输入无效。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            continue;
        }
        clear_input();
        if (exam_id == 0)
            return;

        ExamItemNode *p = exam_item_list;
        while (p != NULL)
        {
            if (p->exam_id == exam_id &&
                p->dept_id == current_user.dept_id)
            {
                chosen = p;
                break;
            }
            p = p->next;
        }
        if (chosen == NULL)
        {
            printf("未找到该检查项目，请从列表中选择。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
        }
    }

    // 创建检查申请节点
    ExamOrderNode *node = (ExamOrderNode *)malloc(sizeof(ExamOrderNode));
    if (node == NULL)
    {
        printf("内存分配失败。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    node->order_id = generate_exam_order_id();
    node->record_id = record_id;
    node->patient_id = patient_id;
    node->exam_id = chosen->exam_id;
    node->price = chosen->price;
    node->status = STATUS_PENDING_PAY;
    node->result = 0;
    node->next = NULL;

    if (exam_order_list == NULL)
    {
        exam_order_list = node;
    }
    else
    {
        ExamOrderNode *tail = exam_order_list;
        while (tail->next != NULL)
            tail = tail->next;
        tail->next = node;
    }

    save_all();
    printf("检查申请已开具！项目：%s，费用：%.1f元\n",
           chosen->exam_name, chosen->price);
    printf("患者缴费后可进行检查。\n");
    printf("按enter键返回...");
    getchar();
}

// 填写检查结果

static void doctor_fill_exam_result()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            填写检查结果                ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 显示已缴费待执行的检查单
    printf("【待填写结果的检查单】\n");
    printf("%-6s %-10s %-16s %-8s\n",
           "申请ID", "患者姓名", "检查项目", "费用");
    printf("------------------------------------------\n");

    ExamOrderNode *cur = exam_order_list;
    int count = 0;
    while (cur != NULL)
    {
        // 找到本科室医生相关的已缴费检查单
        if (cur->status == STATUS_PENDING_DO)
        {
            // 通过record_id找到对应病历确认是本医生的患者
            MedicalRecordNode *rec = record_list;
            while (rec != NULL)
            {
                if (rec->record_id == cur->record_id &&
                    rec->doctor_id == current_user.user_id)
                {
                    // 找检查项目名
                    const char *exam_name = "未知检查";
                    ExamItemNode *ei = exam_item_list;
                    while (ei != NULL)
                    {
                        if (ei->exam_id == cur->exam_id)
                        {
                            exam_name = ei->exam_name;
                            break;
                        }
                        ei = ei->next;
                    }
                    printf("%-6d %-10s %-16s %.1f元\n",
                           cur->order_id,
                           get_patient_name(cur->patient_id),
                           exam_name,
                           cur->price);
                    count++;
                    break;
                }
                rec = rec->next;
            }
        }
        cur = cur->next;
    }

    if (count == 0)
    {
        printf("暂无待填写结果的检查单。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    // 选择检查单
    ExamOrderNode *target = NULL;
    while (target == NULL)
    {
        printf("\n输入要填写结果的申请ID (0=取消): ");
        int order_id;
        if (scanf("%d", &order_id) != 1)
        {
            clear_input();
            printf("输入无效。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            continue;
        }
        clear_input();
        if (order_id == 0)
            return;

        ExamOrderNode *p = exam_order_list;
        while (p != NULL)
        {
            if (p->order_id == order_id &&
                p->status == STATUS_PENDING_DO)
            {
                target = p;
                break;
            }
            p = p->next;
        }
        if (target == NULL)
        {
            printf("未找到该检查单。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
        }
    }
    // 第一次选结果时写入并保存
    printf("请选择检查结果：\n");
    printf("1. 正常，无需处理\n");
    printf("2. 需要开药\n");
    printf("3. 需要住院\n");

    int result = 0;
    while (result < 1 || result > 3)
    {
        printf("请选择 (1-3): ");
        if (scanf("%d", &result) != 1 || result < 1 || result > 3)
        {
            clear_input();
            printf("输入无效。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            result = 0;
            continue;
        }
        clear_input();
    }

    target->result = (ExamResult)result;
    target->status = STATUS_DONE;
    save_all();
    printf("检查结果已填写！\n");

    if (result == 1)
    {
        printf("检查正常，无需处理。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    // result为2或3：执行后循环回到选择，允许追加
    while (1)
    {
        if (result == 2)
            doctor_prescribe(target->record_id, target->patient_id);
        else if (result == 3)
            doctor_apply_inpatient(target->patient_id);

        // 执行完后重新选择
        printf("\n请选择后续操作：\n");
        printf("1. 正常，无需更多处理\n");
        printf("2. 继续开药\n");
        printf("3. 提交住院申请\n");
        printf("0. 返回上级菜单\n");

        result = -1;
        while (result < 0)
        {
            printf("请选择: ");
            if (scanf("%d", &result) != 1 || result < 0 || result > 3)
            {
                clear_input();
                printf("输入无效。\n");
                result = -1;
                continue;
            }
            clear_input();
        }

        if (result == 0 || result == 1)
            break;
    }
}

// 开具处方

static void doctor_prescribe(int record_id, int patient_id)
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            开具处方                    ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 显示本科室和通用药品
    printf("【可开药品列表】\n");
    printf("%-6s %-20s %-8s %-8s\n",
           "药品ID", "药品名称", "单价", "库存");
    printf("------------------------------------------\n");

    DrugNode *drug = drug_list;
    int drug_count = 0;
    while (drug != NULL)
    {
        if (drug->dept_id == current_user.dept_id || drug->dept_id == 0)
        {
            printf("%-6d %-20s %-8.1f %-8d\n",
                   drug->drug_id, drug->drug_name,
                   drug->price, drug->stock);
            drug_count++;
        }
        drug = drug->next;
    }

    if (drug_count == 0)
    {
        printf("暂无可开药品。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    // 选择药品
    DrugNode *chosen_drug = NULL;
    while (chosen_drug == NULL)
    {
        printf("\n请选择药品ID (0=取消): ");
        int drug_id;
        if (scanf("%d", &drug_id) != 1)
        {
            clear_input();
            printf("输入无效。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            continue;
        }
        clear_input();
        if (drug_id == 0)
            return;

        DrugNode *p = drug_list;
        while (p != NULL)
        {
            if (p->drug_id == drug_id &&
                (p->dept_id == current_user.dept_id || p->dept_id == 0))
            {
                chosen_drug = p;
                break;
            }
            p = p->next;
        }
        if (chosen_drug == NULL)
        {
            printf("未找到该药品，请从列表中选择。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
        }
    }

    // 输入数量
    int quantity = 0;
    while (quantity <= 0)
    {
        printf("请输入数量（盒）: ");
        if (scanf("%d", &quantity) != 1 || quantity <= 0)
        {
            clear_input();
            printf("输入无效，数量必须大于0。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            quantity = 0;
            continue;
        }
        clear_input();
        if (quantity > chosen_drug->stock)
        {
            printf("库存不足，当前库存 %d 盒，请重新输入。\n",
                   chosen_drug->stock);
            if (!prompt_yes_no("是否重试？"))
                return;
            quantity = 0;
        }
    }

    // 创建处方节点
    PrescriptionNode *node = (PrescriptionNode *)malloc(sizeof(PrescriptionNode));
    if (node == NULL)
    {
        printf("内存分配失败。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    node->prescription_id = generate_prescription_id();
    node->record_id = record_id;
    node->patient_id = patient_id;
    node->drug_id = chosen_drug->drug_id;
    node->quantity = quantity;
    node->price = chosen_drug->price * quantity;
    node->status = STATUS_PENDING_PAY;
    node->next = NULL;

    if (prescription_list == NULL)
    {
        prescription_list = node;
    }
    else
    {
        PrescriptionNode *tail = prescription_list;
        while (tail->next != NULL)
            tail = tail->next;
        tail->next = node;
    }

    save_all();
    printf("处方已开具！药品：%s × %d盒，总价：%.1f元\n",
           chosen_drug->drug_name, quantity, node->price);
    printf("患者缴费后药剂师可发药。\n");
    printf("按enter键返回...");
    getchar();
}

// 提交住院申请

static void doctor_apply_inpatient(int patient_id)
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            提交住院申请                ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 检查该患者是否已有待审核的住院申请
    InpatientApplyNode *check = inpatient_apply_list;
    while (check != NULL)
    {
        if (check->patient_id == patient_id && check->is_approved == 0)
        {
            printf("该患者已有待审核的住院申请（申请ID：%d），无需重复提交。\n",
                   check->apply_id);
            printf("按enter键返回...");
            getchar();
            return;
        }
        check = check->next;
    }

    printf("患者：%s（病历号%d）\n",
           get_patient_name(patient_id), patient_id);
    printf("科室：%s\n\n", get_dept_name(current_user.dept_id));

    if (!prompt_yes_no("确认提交住院申请？"))
    {
        printf("已取消。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    InpatientApplyNode *node = (InpatientApplyNode *)malloc(sizeof(InpatientApplyNode));
    if (node == NULL)
    {
        printf("内存分配失败。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    node->apply_id = generate_apply_id();
    node->patient_id = patient_id;
    node->doctor_id = current_user.user_id;
    node->dept_id = current_user.dept_id;
    node->is_approved = 0;
    get_now(node->apply_time);
    node->next = NULL;

    if (inpatient_apply_list == NULL)
    {
        inpatient_apply_list = node;
    }
    else
    {
        InpatientApplyNode *tail = inpatient_apply_list;
        while (tail->next != NULL)
            tail = tail->next;
        tail->next = node;
    }

    save_all();
    printf("住院申请已提交！申请ID：%d\n", node->apply_id);
    printf("请等待病房员工审核。\n");
    printf("按enter键返回...");
    getchar();
}

// 接诊患者（核心流程）

void doctor_see_patient()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              接诊患者                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 先触发超时检查
    check_timeout();

    // 显示候诊队列
    printf("【当前候诊队列】\n");
    printf("%-6s %-10s %-6s %-22s\n",
           "挂号ID", "患者姓名", "排队号", "挂号时间");
    printf("----------------------------------------------------\n");

    RegistrationNode *cur = reg_list;
    int count = 0;
    while (cur != NULL)
    {
        if (cur->doctor_id == current_user.user_id &&
            cur->status == STATUS_PENDING_PAY &&
            cur->is_cancelled == 0)
        {
            printf("%-6d %-10s %-6d %-22s\n",
                   cur->reg_id,
                   get_patient_name(cur->patient_id),
                   cur->queue_num,
                   cur->date);
            count++;
        }
        cur = cur->next;
    }

    if (count == 0)
    {
        printf("当前无候诊患者。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    // 选择要接诊的挂号ID
    RegistrationNode *target = NULL;
    while (target == NULL)
    {
        printf("\n输入要接诊的挂号ID (0=取消): ");
        int reg_id;
        if (scanf("%d", &reg_id) != 1)
        {
            clear_input();
            printf("输入无效。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            continue;
        }
        clear_input();
        if (reg_id == 0)
            return;

        RegistrationNode *p = reg_list;
        while (p != NULL)
        {
            if (p->reg_id == reg_id &&
                p->doctor_id == current_user.user_id &&
                p->status == STATUS_PENDING_PAY &&
                p->is_cancelled == 0)
            {
                target = p;
                break;
            }
            p = p->next;
        }
        if (target == NULL)
        {
            printf("未找到该挂号记录，或该号已作废。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
        }
    }

    // 确认接待，挂号状态改为已接诊
    target->status = STATUS_PENDING_DO;
    save_all();

    printf("\n已接诊患者：%s\n\n", get_patient_name(target->patient_id));

    // 填写诊断
    char diagnosis[200];
    printf("请填写诊断内容: ");
    scanf(" %199[^\n]", diagnosis);
    clear_input();

    // 创建病历记录
    MedicalRecordNode *record = (MedicalRecordNode *)malloc(sizeof(MedicalRecordNode));
    if (record == NULL)
    {
        printf("内存分配失败。\n");
        printf("按enter键返回...");
        getchar();
        return;
    }

    record->record_id = generate_record_id();
    record->reg_id = target->reg_id;
    record->patient_id = target->patient_id;
    record->doctor_id = current_user.user_id;
    record->dept_id = current_user.dept_id;
    strncpy(record->diagnosis, diagnosis, sizeof(record->diagnosis) - 1);
    record->diagnosis[sizeof(record->diagnosis) - 1] = '\0';
    get_now(record->date);
    record->next = NULL;

    if (record_list == NULL)
    {
        record_list = record;
    }
    else
    {
        MedicalRecordNode *tail = record_list;
        while (tail->next != NULL)
            tail = tail->next;
        tail->next = record;
    }

    target->status = STATUS_DONE;
    save_all();

    printf("病历已记录（病历ID：%d）\n\n", record->record_id);

    // 选择下一步
    while (1)
    {
        printf("请选择下一步操作：\n");
        printf("1. 开具检查申请\n");
        printf("2. 直接开药\n");
        printf("3. 提交住院申请\n");
        printf("0. 结束本次接诊\n");
        printf("请选择: ");

        int next_step;
        if (scanf("%d", &next_step) != 1)
        {
            clear_input();
            printf("输入无效，请重新输入。\n");
            continue;
        }
        clear_input();

        if (next_step == 0)
            break;

        switch (next_step)
        {
        case 1:
            doctor_order_exam(record->record_id, target->patient_id);
            break;
        case 2:
            doctor_prescribe(record->record_id, target->patient_id);
            break;
        case 3:
            doctor_apply_inpatient(target->patient_id);
            break;
        default:
            printf("输入无效，请重新输入。\n");
            break;
        }
    }

    printf("本次接诊已完成。\n");
    printf("按enter键返回...");
    getchar();
}

// 查看历史病历

void doctor_view_history()
{
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            历史病历查询                ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    int medical_id = 0;
    while (medical_id == 0)
    {
        printf("输入患者病历号 (0=取消): ");
        if (scanf("%d", &medical_id) != 1 || medical_id < 0)
        {
            clear_input();
            printf("输入无效。\n");
            if (!prompt_yes_no("是否重试？"))
                return;
            medical_id = 0;
            continue;
        }
        clear_input();
        if (medical_id == 0)
            return;
    }

    printf("\n患者：%s\n\n", get_patient_name(medical_id));

    printf("%-6s %-16s %-22s %-30s\n",
           "病历ID", "科室", "就诊时间", "诊断");
    printf("------------------------------------------------------------------------\n");

    MedicalRecordNode *cur = record_list;
    int count = 0;
    while (cur != NULL)
    {
        if (cur->patient_id == medical_id &&
            cur->doctor_id == current_user.user_id)
        {
            printf("%-6d %-16s %-22s %-30s\n",
                   cur->record_id,
                   get_dept_name(cur->dept_id),
                   cur->date,
                   cur->diagnosis);
            count++;
        }
        cur = cur->next;
    }

    if (count == 0)
        printf("该患者暂无您接诊的病历记录。\n");

    printf("\n按enter键返回...");
    fflush(stdin);
    getchar();
}

// 医生主菜单

void doctor_menu()
{
    while (1)
    {
        // 每次显示菜单前检查所属科室是否已被停用
        {
            DepartmentNode *chk = dept_list;
            while (chk != NULL)
            {
                if (chk->dept_id == current_user.dept_id)
                {
                    if (chk->is_deleted == 1)
                    {
                        system("cls");
                        printf("╔════════════════════════════════════════╗\n");
                        printf("║              系统通知                  ║\n");
                        printf("╚════════════════════════════════════════╝\n\n");
                        printf("您所属的科室【%s】已被管理员停用。\n", chk->dept_name);
                        printf("您的账号已被强制登出，请联系管理员。\n\n");
                        printf("按enter键返回登录界面...");
                        getchar();
                        current_user.is_logged_in = 0;
                        return;
                    }
                    break;
                }
                chk = chk->next;
            }
        }

        // 每次显示菜单前触发超时检查
        check_timeout();

        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║  欢迎，%-16s                            ║\n", current_user.user_name);
        printf("║  科室：%-16s                            ║\n", get_dept_name(current_user.dept_id));
        printf("╠════════════════════════════════════════╣\n");
        printf("║  1. 接诊患者                           ║\n");
        printf("║  2. 填写检查结果                       ║\n");
        printf("║  3. 患者病历查询                       ║\n");
        printf("║  4. 修改密码                           ║\n");
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

        if (choice < 1 || choice > 4)
        {
            printf("输入无效，请重新输入。\n");
            printf("按enter键继续...");
            getchar();
            continue;
        }

        switch (choice)
        {
        case 1:
            doctor_see_patient();
            break;
        case 2:
            doctor_fill_exam_result();
            break;
        case 3:
            doctor_view_history();
            break;
        case 4:
            change_password();
            break;
        }
    }
}