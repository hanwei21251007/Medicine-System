//韩维组长负责
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/structs.h"

// 数据文件路径
#define FILE_USERS "data/users.txt"
#define FILE_DEPARTMENTS "data/departments.txt"
#define FILE_EXAM_ITEMS "data/exam_items.txt"
#define FILE_DRUGS "data/drugs.txt"
#define FILE_ROOMS "data/rooms.txt"
#define FILE_REGISTRATIONS "data/registrations.txt"
#define FILE_RECORDS "data/medical_records.txt"
#define FILE_EXAM_ORDERS "data/exam_orders.txt"
#define FILE_PRESCRIPTIONS "data/prescriptions.txt"
#define FILE_INPATIENTS "data/inpatients.txt"
#define FILE_INPATIENT_APPLY "data/inpatient_apply.txt"

// 链表头指针（全局）
StaffNode *staff_list = NULL;
PatientNode *patient_list = NULL;
DepartmentNode *dept_list = NULL;
ExamItemNode *exam_item_list = NULL;
DrugNode *drug_list = NULL;
RoomNode *room_list = NULL;
RegistrationNode *reg_list = NULL;
MedicalRecordNode *record_list = NULL;
ExamOrderNode *exam_order_list = NULL;
PrescriptionNode *prescription_list = NULL;
InpatientNode *inpatient_list = NULL;
InpatientApplyNode *inpatient_apply_list = NULL;

// 职称相关工具函数

float get_title_fee(DoctorTitle title)
{
    if (title == TITLE_RESIDENT)
        return 0.0f;
    if (title == TITLE_ATTENDING)
        return 5.0f;
    if (title == TITLE_ASSOCIATE)
        return 15.0f;
    if (title == TITLE_CHIEF)
        return 30.0f;
    return 0.0f;
}

const char *get_title_name(DoctorTitle title)
{
    if (title == TITLE_RESIDENT)
        return "住院医师";
    if (title == TITLE_ATTENDING)
        return "主治医师";
    if (title == TITLE_ASSOCIATE)
        return "副主任医师";
    if (title == TITLE_CHIEF)
        return "主任医师";
    return "????";
}

// 用户数据读写（staff + patient 共用一个文件）

void load_users_from_file()
{
    FILE *fp = fopen(FILE_USERS, "r");
    if (fp == NULL)
    {
        printf("[警告] 未找到用户数据文件\n");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        // 跳过注释和空行
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        // 尝试读员工
        if (line[0] == 'S')
        {
            StaffNode *node = (StaffNode *)malloc(sizeof(StaffNode));
            if (node == NULL)
                continue;
            int role, title, is_deleted;
            if (sscanf(line, "STAFF,%d,%49[^,],%49[^,],%d,%d,%d,%d",
                       &node->id, node->password, node->name,
                       &role, &title, &node->dept_id, &is_deleted) == 7)
            {
                node->role = (UserRole)role;
                node->title = (DoctorTitle)title;
                node->next = NULL;
                node->is_deleted = is_deleted; // 读取时默认正常状态
                // 追加到链表尾
                if (staff_list == NULL)
                {
                    staff_list = node;
                }
                else
                {
                    StaffNode *cur = staff_list;
                    while (cur->next != NULL)
                        cur = cur->next;
                    cur->next = node;
                }
            }
            else
            {
                free(node);
            }
        }

        // 尝试读患者
        if (line[0] == 'P')
        {
            PatientNode *node = (PatientNode *)malloc(sizeof(PatientNode));
            if (node == NULL)
                continue;
            int role;
            if (sscanf(line, "PATIENT,%d,%49[^,],%49[^,],%d,%18[^,],%d",
                       &node->medical_id, node->password, node->name,
                       &role, node->id_card, &node->is_deleted) == 6)
            {
                node->role = (UserRole)role;
                node->next = NULL;
                if (patient_list == NULL)
                {
                    patient_list = node;
                }
                else
                {
                    PatientNode *cur = patient_list;
                    while (cur->next != NULL)
                        cur = cur->next;
                    cur->next = node;
                }
            }
            else
            {
                free(node);
            }
        }
    }
    fclose(fp);
}

void save_users_to_file()
{
    FILE *fp = fopen(FILE_USERS, "w");
    if (fp == NULL)
    {
        printf("[错误] 无法写入用户数据文件\n");
        return;
    }

    fprintf(fp, "# type,id,password,name,role,title,dept_id,is_deleted\n");

    StaffNode *s = staff_list;
    while (s != NULL)
    {
        fprintf(fp, "STAFF,%d,%s,%s,%d,%d,%d,%d\n",
                s->id, s->password, s->name,
                (int)s->role, (int)s->title, s->dept_id, s->is_deleted);
        s = s->next;
    }

    PatientNode *p = patient_list;
    while (p != NULL)
    {
        fprintf(fp, "PATIENT,%d,%s,%s,%d,%s,%d\n",
                p->medical_id, p->password, p->name, (int)p->role, p->id_card, p->is_deleted);
        p = p->next;
    }

    fclose(fp);
}

// 科室数据（只读，不需要写回）

void load_departments()
{
    FILE *fp = fopen(FILE_DEPARTMENTS, "r");
    if (fp == NULL)
    {
        printf("[警告] 未找到科室数据文件\n");
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        DepartmentNode *node = (DepartmentNode *)malloc(sizeof(DepartmentNode));
        if (node == NULL)
            continue;

        if (sscanf(line, "%d,%49[^,],%f,%d",
                   &node->dept_id, node->dept_name, &node->base_reg_fee, &node->is_deleted) == 4)
        {
            node->next = NULL;
            if (dept_list == NULL)
            {
                dept_list = node;
            }
            else
            {
                DepartmentNode *cur = dept_list;
                while (cur->next != NULL)
                    cur = cur->next;
                cur->next = node;
            }
        }
        else
        {
            free(node);
        }
    }
    fclose(fp);
}
void save_departments()
{
    FILE *fp = fopen("data/departments.txt", "w");
    if (fp == NULL)
    {
        printf("无法打开 departments.txt 进行写入。\n");
        return;
    }
    fprintf(fp, "# dept_id,dept_name,base_reg_fee,is_deleted\n");
    DepartmentNode *cur = dept_list;
    while (cur != NULL)
    {
        fprintf(fp, "%d,%s,%.1f,%d\n",
                cur->dept_id, cur->dept_name, cur->base_reg_fee, cur->is_deleted);
        cur = cur->next;
    }
    fclose(fp);
}
// 检查项目（只读）

void load_exam_items()
{
    FILE *fp = fopen(FILE_EXAM_ITEMS, "r");
    if (fp == NULL)
    {
        printf("[警告] 未找到检查项目数据文件\n");
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        ExamItemNode *node = (ExamItemNode *)malloc(sizeof(ExamItemNode));
        if (node == NULL)
            continue;

        if (sscanf(line, "%d,%d,%49[^,],%f",
                   &node->exam_id, &node->dept_id,
                   node->exam_name, &node->price) == 4)
        {
            node->next = NULL;
            if (exam_item_list == NULL)
            {
                exam_item_list = node;
            }
            else
            {
                ExamItemNode *cur = exam_item_list;
                while (cur->next != NULL)
                    cur = cur->next;
                cur->next = node;
            }
        }
        else
        {
            free(node);
        }
    }
    fclose(fp);
}

// 药品数据（库存会变动，需要写回）

void load_drugs()
{
    FILE *fp = fopen(FILE_DRUGS, "r");
    if (fp == NULL)
    {
        printf("[警告] 未找到药品数据文件\n");
        return;
    }

    char line[200];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        DrugNode *node = (DrugNode *)malloc(sizeof(DrugNode));
        if (node == NULL)
            continue;

        if (sscanf(line, "%d,%d,%49[^,],%f,%d,%d",
                   &node->drug_id, &node->dept_id, node->drug_name,
                   &node->price, &node->stock, &node->warning_line) == 6)
        {
            node->next = NULL;
            if (drug_list == NULL)
            {
                drug_list = node;
            }
            else
            {
                DrugNode *cur = drug_list;
                while (cur->next != NULL)
                    cur = cur->next;
                cur->next = node;
            }
        }
        else
        {
            free(node);
        }
    }
    fclose(fp);
}

void save_drugs()
{
    FILE *fp = fopen(FILE_DRUGS, "w");
    if (fp == NULL)
    {
        printf("[错误] 无法写入药品数据文件\n");
        return;
    }

    fprintf(fp, "# drug_id,dept_id,drug_name,price,stock,warning_line\n");

    DrugNode *cur = drug_list;
    while (cur != NULL)
    {
        fprintf(fp, "%d,%d,%s,%.1f,%d,%d\n",
                cur->drug_id, cur->dept_id, cur->drug_name,
                cur->price, cur->stock, cur->warning_line);
        cur = cur->next;
    }
    fclose(fp);
}

// 病房数据（床位占用会变动，需要写回）

void load_rooms()
{
    FILE *fp = fopen(FILE_ROOMS, "r");
    if (fp == NULL)
    {
        printf("[警告] 未找到病房数据文件\n");
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        RoomNode *node = (RoomNode *)malloc(sizeof(RoomNode));
        if (node == NULL)
            continue;

        int room_type;
        if (sscanf(line, "%d,%d,%d,%d,%d,%f",
                   &node->room_id, &node->dept_id, &room_type,
                   &node->capacity, &node->current, &node->daily_fee) == 6)
        {
            node->room_type = (RoomType)room_type;
            node->next = NULL;
            if (room_list == NULL)
            {
                room_list = node;
            }
            else
            {
                RoomNode *cur = room_list;
                while (cur->next != NULL)
                    cur = cur->next;
                cur->next = node;
            }
        }
        else
        {
            free(node);
        }
    }
    fclose(fp);
}

void save_rooms()
{
    FILE *fp = fopen(FILE_ROOMS, "w");
    if (fp == NULL)
    {
        printf("[错误] 无法写入病房数据文件\n");
        return;
    }

    fprintf(fp, "# room_id,dept_id,room_type,capacity,current,daily_fee\n");

    RoomNode *cur = room_list;
    while (cur != NULL)
    {
        fprintf(fp, "%d,%d,%d,%d,%d,%.1f\n",
                cur->room_id, cur->dept_id, (int)cur->room_type,
                cur->capacity, cur->current, cur->daily_fee);
        cur = cur->next;
    }
    fclose(fp);
}

// 挂号记录

void load_registrations()
{
    FILE *fp = fopen(FILE_REGISTRATIONS, "r");
    if (fp == NULL)
        return;

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        RegistrationNode *node = (RegistrationNode *)malloc(sizeof(RegistrationNode));
        if (node == NULL)
            continue;

        int status;
        if (sscanf(line, "%d,%d,%d,%d,%24[^,],%f,%d,%d,%d,%24s",
                   &node->reg_id, &node->patient_id, &node->doctor_id,
                   &node->dept_id, node->date, &node->reg_fee,
                   &node->queue_num, &status,
                   &node->is_cancelled, node->cancel_time) == 10)
        {
            node->status = (ItemStatus)status;
            node->next = NULL;
            if (reg_list == NULL)
            {
                reg_list = node;
            }
            else
            {
                RegistrationNode *cur = reg_list;
                while (cur->next != NULL)
                    cur = cur->next;
                cur->next = node;
            }
        }
        else
        {
            free(node);
        }
    }
    fclose(fp);
}

void save_registrations()
{
    FILE *fp = fopen(FILE_REGISTRATIONS, "w");
    if (fp == NULL)
        return;

    fprintf(fp, "# reg_id,patient_id,doctor_id,dept_id,date,reg_fee,queue_num,status\n");

    RegistrationNode *cur = reg_list;
    while (cur != NULL)
    {
        fprintf(fp, "%d,%d,%d,%d,%s,%.1f,%d,%d,%d,%s\n",
                cur->reg_id, cur->patient_id, cur->doctor_id,
                cur->dept_id, cur->date, cur->reg_fee,
                cur->queue_num, (int)cur->status,
                cur->is_cancelled, cur->cancel_time);
        cur = cur->next;
    }
    fclose(fp);
}

// 病历记录

void load_medical_records()
{
    FILE *fp = fopen(FILE_RECORDS, "r");
    if (fp == NULL)
        return;

    char line[512];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        MedicalRecordNode *node = (MedicalRecordNode *)malloc(sizeof(MedicalRecordNode));
        if (node == NULL)
            continue;

        if (sscanf(line, "%d,%d,%d,%d,%d,%199[^,],%19s",
                   &node->record_id, &node->reg_id, &node->patient_id,
                   &node->doctor_id, &node->dept_id,
                   node->diagnosis, node->date) == 7)
        {
            node->next = NULL;
            if (record_list == NULL)
            {
                record_list = node;
            }
            else
            {
                MedicalRecordNode *cur = record_list;
                while (cur->next != NULL)
                    cur = cur->next;
                cur->next = node;
            }
        }
        else
        {
            free(node);
        }
    }
    fclose(fp);
}

void save_medical_records()
{
    FILE *fp = fopen(FILE_RECORDS, "w");
    if (fp == NULL)
        return;

    fprintf(fp, "# record_id,reg_id,patient_id,doctor_id,dept_id,diagnosis,date\n");

    MedicalRecordNode *cur = record_list;
    while (cur != NULL)
    {
        fprintf(fp, "%d,%d,%d,%d,%d,%s,%s\n",
                cur->record_id, cur->reg_id, cur->patient_id,
                cur->doctor_id, cur->dept_id,
                cur->diagnosis, cur->date);
        cur = cur->next;
    }
    fclose(fp);
}

// 检查申请单

void load_exam_orders()
{
    FILE *fp = fopen(FILE_EXAM_ORDERS, "r");
    if (fp == NULL)
        return;

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        ExamOrderNode *node = (ExamOrderNode *)malloc(sizeof(ExamOrderNode));
        if (node == NULL)
            continue;

        int status, result;
        if (sscanf(line, "%d,%d,%d,%d,%f,%d,%d",
                   &node->order_id, &node->record_id, &node->patient_id,
                   &node->exam_id, &node->price, &status, &result) == 7)
        {
            node->status = (ItemStatus)status;
            node->result = (ExamResult)result;
            node->next = NULL;
            if (exam_order_list == NULL)
            {
                exam_order_list = node;
            }
            else
            {
                ExamOrderNode *cur = exam_order_list;
                while (cur->next != NULL)
                    cur = cur->next;
                cur->next = node;
            }
        }
        else
        {
            free(node);
        }
    }
    fclose(fp);
}

void save_exam_orders()
{
    FILE *fp = fopen(FILE_EXAM_ORDERS, "w");
    if (fp == NULL)
        return;

    fprintf(fp, "# order_id,record_id,patient_id,exam_id,price,status,result\n");

    ExamOrderNode *cur = exam_order_list;
    while (cur != NULL)
    {
        fprintf(fp, "%d,%d,%d,%d,%.1f,%d,%d\n",
                cur->order_id, cur->record_id, cur->patient_id,
                cur->exam_id, cur->price,
                (int)cur->status, (int)cur->result);
        cur = cur->next;
    }
    fclose(fp);
}

// 处方

void load_prescriptions()
{
    FILE *fp = fopen(FILE_PRESCRIPTIONS, "r");
    if (fp == NULL)
        return;

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        PrescriptionNode *node = (PrescriptionNode *)malloc(sizeof(PrescriptionNode));
        if (node == NULL)
            continue;

        int status;
        if (sscanf(line, "%d,%d,%d,%d,%d,%f,%d",
                   &node->prescription_id, &node->record_id, &node->patient_id,
                   &node->drug_id, &node->quantity, &node->price, &status) == 7)
        {
            node->status = (ItemStatus)status;
            node->next = NULL;
            if (prescription_list == NULL)
            {
                prescription_list = node;
            }
            else
            {
                PrescriptionNode *cur = prescription_list;
                while (cur->next != NULL)
                    cur = cur->next;
                cur->next = node;
            }
        }
        else
        {
            free(node);
        }
    }
    fclose(fp);
}

void save_prescriptions()
{
    FILE *fp = fopen(FILE_PRESCRIPTIONS, "w");
    if (fp == NULL)
        return;

    fprintf(fp, "# prescription_id,record_id,patient_id,drug_id,quantity,price,status\n");

    PrescriptionNode *cur = prescription_list;
    while (cur != NULL)
    {
        fprintf(fp, "%d,%d,%d,%d,%d,%.1f,%d\n",
                cur->prescription_id, cur->record_id, cur->patient_id,
                cur->drug_id, cur->quantity, cur->price, (int)cur->status);
        cur = cur->next;
    }
    fclose(fp);
}

// 住院记录

void load_inpatients()
{
    FILE *fp = fopen(FILE_INPATIENTS, "r");
    if (fp == NULL)
        return;

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        InpatientNode *node = (InpatientNode *)malloc(sizeof(InpatientNode));
        if (node == NULL)
            continue;

        if (sscanf(line, "%d,%d,%d,%d,%d,%d,%19[^,],%19[^,],%d,%f,%d",
                   &node->inpatient_id, &node->patient_id, &node->doctor_id,
                   &node->dept_id, &node->room_id, &node->bed_num,
                   node->admit_date, node->discharge_date,
                   &node->days, &node->total_fee, &node->is_discharged) == 11)
        {
            node->next = NULL;
            if (inpatient_list == NULL)
            {
                inpatient_list = node;
            }
            else
            {
                InpatientNode *cur = inpatient_list;
                while (cur->next != NULL)
                    cur = cur->next;
                cur->next = node;
            }
        }
        else
        {
            free(node);
        }
    }
    fclose(fp);
}

void save_inpatients()
{
    FILE *fp = fopen(FILE_INPATIENTS, "w");
    if (fp == NULL)
        return;

    fprintf(fp, "# inpatient_id,patient_id,doctor_id,dept_id,room_id,bed_num,admit_date,discharge_date,days,total_fee,is_discharged\n");

    InpatientNode *cur = inpatient_list;
    while (cur != NULL)
    {
        fprintf(fp, "%d,%d,%d,%d,%d,%d,%s,%s,%d,%.1f,%d\n",
                cur->inpatient_id, cur->patient_id, cur->doctor_id,
                cur->dept_id, cur->room_id, cur->bed_num,
                cur->admit_date, cur->discharge_date,
                cur->days, cur->total_fee, cur->is_discharged);
        cur = cur->next;
    }
    fclose(fp);
}

void load_inpatient_apply()
{
    FILE *fp = fopen(FILE_INPATIENT_APPLY, "r");
    if (fp == NULL)
        return;

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        InpatientApplyNode *node = (InpatientApplyNode *)malloc(sizeof(InpatientApplyNode));
        if (node == NULL)
            continue;

        if (sscanf(line, "%d,%d,%d,%d,%24[^,],%d",
                   &node->apply_id, &node->patient_id, &node->doctor_id,
                   &node->dept_id, node->apply_time, &node->is_approved) == 6)
        {
            node->next = NULL;
            if (inpatient_apply_list == NULL)
            {
                inpatient_apply_list = node;
            }
            else
            {
                InpatientApplyNode *cur = inpatient_apply_list;
                while (cur->next != NULL)
                    cur = cur->next;
                cur->next = node;
            }
        }
        else
        {
            free(node);
        }
    }
    fclose(fp);
}

void save_inpatient_apply()
{
    FILE *fp = fopen(FILE_INPATIENT_APPLY, "w");
    if (fp == NULL)
        return;

    fprintf(fp, "# apply_id,patient_id,doctor_id,dept_id,apply_time,is_approved\n");

    InpatientApplyNode *cur = inpatient_apply_list;
    while (cur != NULL)
    {
        fprintf(fp, "%d,%d,%d,%d,%s,%d\n",
                cur->apply_id, cur->patient_id, cur->doctor_id,
                cur->dept_id, cur->apply_time, cur->is_approved);
        cur = cur->next;
    }
    fclose(fp);
}

// ID 自增生成器

int generate_medical_id()
{
    int max = 20000;
    PatientNode *cur = patient_list;
    while (cur != NULL)
    {
        if (cur->medical_id > max)
            max = cur->medical_id;
        cur = cur->next;
    }
    return max + 1;
}

int generate_reg_id()
{
    int max = 0;
    RegistrationNode *cur = reg_list;
    while (cur != NULL)
    {
        if (cur->reg_id > max)
            max = cur->reg_id;
        cur = cur->next;
    }
    return max + 1;
}

int generate_record_id()
{
    int max = 0;
    MedicalRecordNode *cur = record_list;
    while (cur != NULL)
    {
        if (cur->record_id > max)
            max = cur->record_id;
        cur = cur->next;
    }
    return max + 1;
}

int generate_exam_order_id()
{
    int max = 0;
    ExamOrderNode *cur = exam_order_list;
    while (cur != NULL)
    {
        if (cur->order_id > max)
            max = cur->order_id;
        cur = cur->next;
    }
    return max + 1;
}

int generate_prescription_id()
{
    int max = 0;
    PrescriptionNode *cur = prescription_list;
    while (cur != NULL)
    {
        if (cur->prescription_id > max)
            max = cur->prescription_id;
        cur = cur->next;
    }
    return max + 1;
}

int generate_inpatient_id()
{
    int max = 0;
    InpatientNode *cur = inpatient_list;
    while (cur != NULL)
    {
        if (cur->inpatient_id > max)
            max = cur->inpatient_id;
        cur = cur->next;
    }
    return max + 1;
}

int generate_apply_id()
{
    int max = 0;
    InpatientApplyNode *cur = inpatient_apply_list;
    while (cur != NULL)
    {
        if (cur->apply_id > max)
            max = cur->apply_id;
        cur = cur->next;
    }
    return max + 1;
}

// 获取指定科室当前排队人数

int get_queue_count(int dept_id)
{
    int count = 0;
    RegistrationNode *cur = reg_list;
    while (cur != NULL)
    {
        if (cur->dept_id == dept_id && cur->status == STATUS_PENDING_PAY)
            count++;
        cur = cur->next;
    }
    return count;
}

// 统一启动加载 / 退出保存 / 释放内存

void load_all()
{
    load_users_from_file();
    load_departments();
    load_exam_items();
    load_drugs();
    load_rooms();
    load_registrations();
    load_medical_records();
    load_exam_orders();
    load_prescriptions();
    load_inpatients();
    load_inpatient_apply();
}

void save_all()
{
    save_users_to_file();
    save_departments();
    save_drugs();
    save_rooms();
    save_registrations();
    save_medical_records();
    save_exam_orders();
    save_prescriptions();
    save_inpatients();
    save_inpatient_apply();
}

void free_all_lists()
{
    StaffNode *s = staff_list;
    while (s != NULL)
    {
        StaffNode *tmp = s;
        s = s->next;
        free(tmp);
    }
    staff_list = NULL;

    PatientNode *p = patient_list;
    while (p != NULL)
    {
        PatientNode *tmp = p;
        p = p->next;
        free(tmp);
    }
    patient_list = NULL;

    DepartmentNode *d = dept_list;
    while (d != NULL)
    {
        DepartmentNode *tmp = d;
        d = d->next;
        free(tmp);
    }
    dept_list = NULL;

    ExamItemNode *e = exam_item_list;
    while (e != NULL)
    {
        ExamItemNode *tmp = e;
        e = e->next;
        free(tmp);
    }
    exam_item_list = NULL;

    DrugNode *dr = drug_list;
    while (dr != NULL)
    {
        DrugNode *tmp = dr;
        dr = dr->next;
        free(tmp);
    }
    drug_list = NULL;

    RoomNode *r = room_list;
    while (r != NULL)
    {
        RoomNode *tmp = r;
        r = r->next;
        free(tmp);
    }
    room_list = NULL;

    RegistrationNode *rg = reg_list;
    while (rg != NULL)
    {
        RegistrationNode *tmp = rg;
        rg = rg->next;
        free(tmp);
    }
    reg_list = NULL;

    MedicalRecordNode *mr = record_list;
    while (mr != NULL)
    {
        MedicalRecordNode *tmp = mr;
        mr = mr->next;
        free(tmp);
    }
    record_list = NULL;

    ExamOrderNode *eo = exam_order_list;
    while (eo != NULL)
    {
        ExamOrderNode *tmp = eo;
        eo = eo->next;
        free(tmp);
    }
    exam_order_list = NULL;

    PrescriptionNode *pr = prescription_list;
    while (pr != NULL)
    {
        PrescriptionNode *tmp = pr;
        pr = pr->next;
        free(tmp);
    }
    prescription_list = NULL;

    InpatientNode *ip = inpatient_list;
    while (ip != NULL)
    {
        InpatientNode *tmp = ip;
        ip = ip->next;
        free(tmp);
    }
    inpatient_list = NULL;

    InpatientApplyNode *ia = inpatient_apply_list;
    while (ia != NULL)
    {
        InpatientApplyNode *tmp = ia;
        ia = ia->next;
        free(tmp);
    }
    inpatient_apply_list = NULL;
}

// 输入缓冲区清空
void clear_input()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

// 获取用户选择
int get_choice(int min_choice, int max_choice)
{
    char buf[64];
    int choice;

    while (1)
    {
        // 读取整行，防止残留字符污染下次输入
        if (fgets(buf, sizeof(buf), stdin) == NULL)
        {
            // EOF / 流错误：安全退出
            return min_choice - 1;
        }

        // 截断过长行（缓冲区末尾没有 '\n' 说明行被截断）
        size_t len = strlen(buf);
        if (len > 0 && buf[len - 1] != '\n')
        {
            // 丢弃剩余输入直到换行
            int c;
            while ((c = getchar()) != '\n' && c != EOF)
                ;
            printf("  输入过长，请重新输入: ");
            continue;
        }

        // 去掉末尾换行
        buf[strcspn(buf, "\n")] = '\0';

        // 去除首尾空格，提取中间内容
        const char *p = buf;
        while (*p == ' ')
            p++; // 跳过前导空格

        const char *end = p + strlen(p) - 1;
        while (end >= p && *end == ' ')
            end--; // 去掉尾部空格

        // 空行
        if (end < p)
        {
            printf("  输入不能为空，请重新输入: ");
            continue;
        }

        // 检查中间（去空格后）是否全为数字
        int digit_count = 0;
        int valid = 1;
        for (const char *q = p; q <= end; q++)
        {
            if (isdigit((unsigned char)*q))
            {
                digit_count++;
            }
            else
            {
                // 中间出现非数字字符（含中间空格）
                valid = 0;
                break;
            }
        }

        if (!valid || digit_count == 0)
        {
            printf("  无效输入，请只输入数字序号: ");
            continue;
        }

        // 转换并范围检查
        choice = atoi(p);
        if (choice < min_choice || choice > max_choice)
        {
            printf("  序号超出范围（%d-%d），请重新输入: ",
                   min_choice, max_choice);
            continue;
        }

        return choice;
    }
}

// yes/no 选择
int prompt_yes_no(const char *prompt)
{
    char c;
    while (1)
    {
        printf("\n%s (y/n): ", prompt);
        c = getchar();

        // 检查后面是否还有多余字符
        char next = getchar();
        if (next != '\n')
        {
            // 有多余字符，清空缓冲区，判无效
            while (getchar() != '\n')
                ;
            printf("输入无效，请输入 y 或 n。");
            continue;
        }

        if (c == 'y' || c == 'Y')
            return 1;
        if (c == 'n' || c == 'N')
            return 0;
        printf("输入无效，请输入 y 或 n。");
    }
}

// 名字是否合法
int is_valid_name(const char *s)
{
    int len = strlen(s);
    if (len == 0)
        return 0;

    int i = 0;
    int last_was_space = 0;
    int has_chinese = 0, has_english = 0;

    while (i < len)
    {
        unsigned char ch = (unsigned char)s[i];

        if (ch >= 0x81 && ch <= 0xFE)
        {
            if (i + 1 >= len)
                return 0;
            unsigned char ch2 = (unsigned char)s[i + 1];
            if (ch2 < 0x40 || ch2 > 0xFE)
                return 0;
            has_chinese = 1;
            last_was_space = 0;
            i += 2;
        }
        else if (isalpha(ch))
        {
            has_english = 1;
            last_was_space = 0;
            i++;
        }
        else if (ch == ' ')
        {
            if (has_chinese)
                return 0;
            if (last_was_space)
                return 0;
            if (i == 0)
                return 0;
            last_was_space = 1;
            i++;
        }
        else
        {
            return 0;
        }
    }

    if (last_was_space)
        return 0;
    if (has_chinese && has_english)
        return 0;
    return 1;
}

int is_valid_password(const char *s)
{
    if (strlen(s) == 0)
        return 0;
    for (int i = 0; s[i]; i++)
        if (s[i] == ' ')
            return 0;
    return 1;
}
void change_password()
{   
    extern CurrentUser current_user;
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              修改密码                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    char old_pwd[50], new_pwd[50], confirm_pwd[50];

    // 第一步：验证旧密码
    printf("请输入当前密码: ");
    fgets(old_pwd, sizeof(old_pwd), stdin);
    old_pwd[strcspn(old_pwd, "\r\n")] = '\0';
    int verified = 0;

    if (current_user.user_role == ROLE_PATIENT)
    {
        PatientNode *cur = patient_list;
        while (cur != NULL)
        {
            if (cur->medical_id == current_user.user_id)
            {
                if (strcmp(cur->password, old_pwd) == 0)
                    verified = 1;
                break;
            }
            cur = cur->next;
        }
    }
    else
    {
        StaffNode *cur = staff_list;
        while (cur != NULL)
        {
            if (cur->id == current_user.user_id)
            {
                if (strcmp(cur->password, old_pwd) == 0)
                    verified = 1;
                break;
            }
            cur = cur->next;
        }
    }

    if (!verified)
    {
        printf("当前密码错误，无法修改。\n");
        if (!prompt_yes_no("是否重新尝试？"))
            return;
        // 重试：递归调用自身重新走完整流程
        change_password();
        return;
    }

    // 第二步：输入并确认新密码
    while (1)
    {
        printf("请输入新密码: ");
        fgets(new_pwd, sizeof(new_pwd), stdin);
        new_pwd[strcspn(new_pwd, "\r\n")] = '\0';

        if (!is_valid_password(new_pwd))
        {
            printf("密码不合法（不能为空或包含空格），请重新输入。\n");
            if (!prompt_yes_no("是否重新尝试？"))
                return;
            continue;
        }

        if (strcmp(new_pwd, old_pwd) == 0)
        {
            printf("新密码不能与当前密码相同，请重新输入。\n");
            if (!prompt_yes_no("是否重新尝试？"))
                return;
            continue;
        }

        printf("请再次输入新密码确认: ");
        fgets(confirm_pwd, sizeof(confirm_pwd), stdin);
        confirm_pwd[strcspn(confirm_pwd, "\r\n")] = '\0';

        if (strcmp(new_pwd, confirm_pwd) != 0)
        {
            printf("两次输入不一致，请重新输入。\n");
            if (!prompt_yes_no("是否重新尝试？"))
                return;
            continue;
        }
        break;
    }

    // 第三步：写入链表并持久化保存
    if (current_user.user_role == ROLE_PATIENT)
    {
        PatientNode *cur = patient_list;
        while (cur != NULL)
        {
            if (cur->medical_id == current_user.user_id)
            {
                strncpy(cur->password, new_pwd, sizeof(cur->password) - 1);
                cur->password[sizeof(cur->password) - 1] = '\0';
                break;
            }
            cur = cur->next;
        }
    }
    else
    {
        StaffNode *cur = staff_list;
        while (cur != NULL)
        {
            if (cur->id == current_user.user_id)
            {
                strncpy(cur->password, new_pwd, sizeof(cur->password) - 1);
                cur->password[sizeof(cur->password) - 1] = '\0';
                break;
            }
            cur = cur->next;
        }
    }

    save_all();
    printf("\n密码修改成功！下次登录请使用新密码。\n");
    printf("按enter键返回...");
    getchar();
}