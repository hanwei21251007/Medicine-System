#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/structs.h"

// 数据文件路径
#define FILE_USERS          "data/users.txt"
#define FILE_DEPARTMENTS    "data/departments.txt"
#define FILE_EXAM_ITEMS     "data/exam_items.txt"
#define FILE_DRUGS          "data/drugs.txt"
#define FILE_ROOMS          "data/rooms.txt"
#define FILE_REGISTRATIONS  "data/registrations.txt"
#define FILE_RECORDS        "data/medical_records.txt"
#define FILE_EXAM_ORDERS    "data/exam_orders.txt"
#define FILE_PRESCRIPTIONS  "data/prescriptions.txt"
#define FILE_INPATIENTS     "data/inpatients.txt"

// 链表头指针（全局）
StaffNode         *staff_list        = NULL;
PatientNode       *patient_list      = NULL;
DepartmentNode    *dept_list         = NULL;
ExamItemNode      *exam_item_list    = NULL;
DrugNode          *drug_list         = NULL;
RoomNode          *room_list         = NULL;
RegistrationNode  *reg_list          = NULL;
MedicalRecordNode *record_list       = NULL;
ExamOrderNode     *exam_order_list   = NULL;
PrescriptionNode  *prescription_list = NULL;
InpatientNode     *inpatient_list    = NULL;


// 职称相关工具函数

float get_title_fee(DoctorTitle title) {
    if (title == TITLE_RESIDENT)  return 0.0f;
    if (title == TITLE_ATTENDING) return 5.0f;
    if (title == TITLE_ASSOCIATE) return 15.0f;
    if (title == TITLE_CHIEF)     return 30.0f;
    return 0.0f;
}

const char *get_title_name(DoctorTitle title) {
    if (title == TITLE_RESIDENT)  return "住院医师";
    if (title == TITLE_ATTENDING) return "主治医师";
    if (title == TITLE_ASSOCIATE) return "副主任医师";
    if (title == TITLE_CHIEF)     return "主任医师";
    return "未知";
}

// 用户数据读写（staff + patient 共用一个文件）


void load_users_from_file() {
    FILE *fp = fopen(FILE_USERS, "r");
    if (fp == NULL) {
        printf("[警告] 未找到用户数据文件\n");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        // 跳过注释和空行
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        // 尝试读员工
        if (line[0] == 'S') {
            StaffNode *node = (StaffNode *)malloc(sizeof(StaffNode));
            if (node == NULL) continue;
            int role, title;
            if (sscanf(line, "STAFF,%d,%49[^,],%49[^,],%d,%d,%d,",
                       &node->id, node->password, node->name,
                       &role, &title, &node->dept_id) == 6) {
                node->role  = (UserRole)role;
                node->title = (DoctorTitle)title;
                node->next  = NULL;
                // 追加到链表尾
                if (staff_list == NULL) {
                    staff_list = node;
                } else {
                    StaffNode *cur = staff_list;
                    while (cur->next != NULL) cur = cur->next;
                    cur->next = node;
                }
            } else {
                free(node);
            }
        }

        // 尝试读患者
        if (line[0] == 'P') {
            PatientNode *node = (PatientNode *)malloc(sizeof(PatientNode));
            if (node == NULL) continue;
            int role, title, dept;
            if (sscanf(line, "PATIENT,%d,%49[^,],%49[^,],%d,%d,%d,%18s",
                       &node->medical_id, node->password, node->name,
                       &role, &title, &dept, node->id_card) == 7) {
                node->gender = 0;
                node->age    = 0;
                node->next   = NULL;
                if (patient_list == NULL) {
                    patient_list = node;
                } else {
                    PatientNode *cur = patient_list;
                    while (cur->next != NULL) cur = cur->next;
                    cur->next = node;
                }
            } else {
                free(node);
            }
        }
    }
    fclose(fp);
}

void save_users_to_file() {
    FILE *fp = fopen(FILE_USERS, "w");
    if (fp == NULL) {
        printf("[错误] 无法写入用户数据文件\n");
        return;
    }

    fprintf(fp, "# type,id,password,name,role,title,dept_id,id_card\n");

    StaffNode *s = staff_list;
    while (s != NULL) {
        fprintf(fp, "STAFF,%d,%s,%s,%d,%d,%d,\n",
                s->id, s->password, s->name,
                (int)s->role, (int)s->title, s->dept_id);
        s = s->next;
    }

    PatientNode *p = patient_list;
    while (p != NULL) {
        fprintf(fp, "PATIENT,%d,%s,%s,4,0,0,%s\n",
                p->medical_id, p->password, p->name, p->id_card);
        p = p->next;
    }

    fclose(fp);
}

// 科室数据（只读，不需要写回）

void load_departments() {
    FILE *fp = fopen(FILE_DEPARTMENTS, "r");
    if (fp == NULL) {
        printf("[警告] 未找到科室数据文件\n");
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        DepartmentNode *node = (DepartmentNode *)malloc(sizeof(DepartmentNode));
        if (node == NULL) continue;

        if (sscanf(line, "%d,%49[^,],%f",
                   &node->dept_id, node->dept_name, &node->base_reg_fee) == 3) {
            node->next = NULL;
            if (dept_list == NULL) {
                dept_list = node;
            } else {
                DepartmentNode *cur = dept_list;
                while (cur->next != NULL) cur = cur->next;
                cur->next = node;
            }
        } else {
            free(node);
        }
    }
    fclose(fp);
}

// 检查项目（只读）

void load_exam_items() {
    FILE *fp = fopen(FILE_EXAM_ITEMS, "r");
    if (fp == NULL) {
        printf("[警告] 未找到检查项目数据文件\n");
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        ExamItemNode *node = (ExamItemNode *)malloc(sizeof(ExamItemNode));
        if (node == NULL) continue;

        if (sscanf(line, "%d,%d,%49[^,],%f",
                   &node->exam_id, &node->dept_id,
                   node->exam_name, &node->price) == 4) {
            node->next = NULL;
            if (exam_item_list == NULL) {
                exam_item_list = node;
            } else {
                ExamItemNode *cur = exam_item_list;
                while (cur->next != NULL) cur = cur->next;
                cur->next = node;
            }
        } else {
            free(node);
        }
    }
    fclose(fp);
}

// 药品数据（库存会变动，需要写回）

void load_drugs() {
    FILE *fp = fopen(FILE_DRUGS, "r");
    if (fp == NULL) {
        printf("[警告] 未找到药品数据文件\n");
        return;
    }

    char line[200];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        DrugNode *node = (DrugNode *)malloc(sizeof(DrugNode));
        if (node == NULL) continue;

        if (sscanf(line, "%d,%d,%49[^,],%f,%d,%d",
                   &node->drug_id, &node->dept_id, node->drug_name,
                   &node->price, &node->stock, &node->warning_line) == 6) {
            node->next = NULL;
            if (drug_list == NULL) {
                drug_list = node;
            } else {
                DrugNode *cur = drug_list;
                while (cur->next != NULL) cur = cur->next;
                cur->next = node;
            }
        } else {
            free(node);
        }
    }
    fclose(fp);
}

void save_drugs() {
    FILE *fp = fopen(FILE_DRUGS, "w");
    if (fp == NULL) {
        printf("[错误] 无法写入药品数据文件\n");
        return;
    }

    fprintf(fp, "# drug_id,dept_id,drug_name,price,stock,warning_line\n");

    DrugNode *cur = drug_list;
    while (cur != NULL) {
        fprintf(fp, "%d,%d,%s,%.1f,%d,%d\n",
                cur->drug_id, cur->dept_id, cur->drug_name,
                cur->price, cur->stock, cur->warning_line);
        cur = cur->next;
    }
    fclose(fp);
}

// 病房数据（床位占用会变动，需要写回）

void load_rooms() {
    FILE *fp = fopen(FILE_ROOMS, "r");
    if (fp == NULL) {
        printf("[警告] 未找到病房数据文件\n");
        return;
    }

    char line[128];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        RoomNode *node = (RoomNode *)malloc(sizeof(RoomNode));
        if (node == NULL) continue;

        int room_type;
        if (sscanf(line, "%d,%d,%d,%d,%d,%f",
                   &node->room_id, &node->dept_id, &room_type,
                   &node->capacity, &node->current, &node->daily_fee) == 6) {
            node->room_type = (RoomType)room_type;
            node->next = NULL;
            if (room_list == NULL) {
                room_list = node;
            } else {
                RoomNode *cur = room_list;
                while (cur->next != NULL) cur = cur->next;
                cur->next = node;
            }
        } else {
            free(node);
        }
    }
    fclose(fp);
}

void save_rooms() {
    FILE *fp = fopen(FILE_ROOMS, "w");
    if (fp == NULL) {
        printf("[错误] 无法写入病房数据文件\n");
        return;
    }

    fprintf(fp, "# room_id,dept_id,room_type,capacity,current,daily_fee\n");

    RoomNode *cur = room_list;
    while (cur != NULL) {
        fprintf(fp, "%d,%d,%d,%d,%d,%.1f\n",
                cur->room_id, cur->dept_id, (int)cur->room_type,
                cur->capacity, cur->current, cur->daily_fee);
        cur = cur->next;
    }
    fclose(fp);
}

// 挂号记录

void load_registrations() {
    FILE *fp = fopen(FILE_REGISTRATIONS, "r");
    if (fp == NULL) return;

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        RegistrationNode *node = (RegistrationNode *)malloc(sizeof(RegistrationNode));
        if (node == NULL) continue;

        int status;
        if (sscanf(line, "%d,%d,%d,%d,%19[^,],%f,%d,%d",
                   &node->reg_id, &node->patient_id, &node->doctor_id,
                   &node->dept_id, node->date, &node->reg_fee,
                   &node->queue_num, &status) == 8) {
            node->status = (ItemStatus)status;
            node->next = NULL;
            if (reg_list == NULL) {
                reg_list = node;
            } else {
                RegistrationNode *cur = reg_list;
                while (cur->next != NULL) cur = cur->next;
                cur->next = node;
            }
        } else {
            free(node);
        }
    }
    fclose(fp);
}

void save_registrations() {
    FILE *fp = fopen(FILE_REGISTRATIONS, "w");
    if (fp == NULL) return;

    fprintf(fp, "# reg_id,patient_id,doctor_id,dept_id,date,reg_fee,queue_num,status\n");

    RegistrationNode *cur = reg_list;
    while (cur != NULL) {
        fprintf(fp, "%d,%d,%d,%d,%s,%.1f,%d,%d\n",
                cur->reg_id, cur->patient_id, cur->doctor_id,
                cur->dept_id, cur->date, cur->reg_fee,
                cur->queue_num, (int)cur->status);
        cur = cur->next;
    }
    fclose(fp);
}

// 病历记录

void load_medical_records() {
    FILE *fp = fopen(FILE_RECORDS, "r");
    if (fp == NULL) return;

    char line[512];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        MedicalRecordNode *node = (MedicalRecordNode *)malloc(sizeof(MedicalRecordNode));
        if (node == NULL) continue;

        if (sscanf(line, "%d,%d,%d,%d,%d,%199[^,],%19s",
                   &node->record_id, &node->reg_id, &node->patient_id,
                   &node->doctor_id, &node->dept_id,
                   node->diagnosis, node->date) == 7) {
            node->next = NULL;
            if (record_list == NULL) {
                record_list = node;
            } else {
                MedicalRecordNode *cur = record_list;
                while (cur->next != NULL) cur = cur->next;
                cur->next = node;
            }
        } else {
            free(node);
        }
    }
    fclose(fp);
}

void save_medical_records() {
    FILE *fp = fopen(FILE_RECORDS, "w");
    if (fp == NULL) return;

    fprintf(fp, "# record_id,reg_id,patient_id,doctor_id,dept_id,diagnosis,date\n");

    MedicalRecordNode *cur = record_list;
    while (cur != NULL) {
        fprintf(fp, "%d,%d,%d,%d,%d,%s,%s\n",
                cur->record_id, cur->reg_id, cur->patient_id,
                cur->doctor_id, cur->dept_id,
                cur->diagnosis, cur->date);
        cur = cur->next;
    }
    fclose(fp);
}

// 检查申请单

void load_exam_orders() {
    FILE *fp = fopen(FILE_EXAM_ORDERS, "r");
    if (fp == NULL) return;

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        ExamOrderNode *node = (ExamOrderNode *)malloc(sizeof(ExamOrderNode));
        if (node == NULL) continue;

        int status, result;
        if (sscanf(line, "%d,%d,%d,%d,%f,%d,%d",
                   &node->order_id, &node->record_id, &node->patient_id,
                   &node->exam_id, &node->price, &status, &result) == 7) {
            node->status = (ItemStatus)status;
            node->result = (ExamResult)result;
            node->next = NULL;
            if (exam_order_list == NULL) {
                exam_order_list = node;
            } else {
                ExamOrderNode *cur = exam_order_list;
                while (cur->next != NULL) cur = cur->next;
                cur->next = node;
            }
        } else {
            free(node);
        }
    }
    fclose(fp);
}

void save_exam_orders() {
    FILE *fp = fopen(FILE_EXAM_ORDERS, "w");
    if (fp == NULL) return;

    fprintf(fp, "# order_id,record_id,patient_id,exam_id,price,status,result\n");

    ExamOrderNode *cur = exam_order_list;
    while (cur != NULL) {
        fprintf(fp, "%d,%d,%d,%d,%.1f,%d,%d\n",
                cur->order_id, cur->record_id, cur->patient_id,
                cur->exam_id, cur->price,
                (int)cur->status, (int)cur->result);
        cur = cur->next;
    }
    fclose(fp);
}

// 处方

void load_prescriptions() {
    FILE *fp = fopen(FILE_PRESCRIPTIONS, "r");
    if (fp == NULL) return;

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        PrescriptionNode *node = (PrescriptionNode *)malloc(sizeof(PrescriptionNode));
        if (node == NULL) continue;

        int status;
        if (sscanf(line, "%d,%d,%d,%d,%d,%f,%d",
                   &node->prescription_id, &node->record_id, &node->patient_id,
                   &node->drug_id, &node->quantity, &node->price, &status) == 7) {
            node->status = (ItemStatus)status;
            node->next = NULL;
            if (prescription_list == NULL) {
                prescription_list = node;
            } else {
                PrescriptionNode *cur = prescription_list;
                while (cur->next != NULL) cur = cur->next;
                cur->next = node;
            }
        } else {
            free(node);
        }
    }
    fclose(fp);
}

void save_prescriptions() {
    FILE *fp = fopen(FILE_PRESCRIPTIONS, "w");
    if (fp == NULL) return;

    fprintf(fp, "# prescription_id,record_id,patient_id,drug_id,quantity,price,status\n");

    PrescriptionNode *cur = prescription_list;
    while (cur != NULL) {
        fprintf(fp, "%d,%d,%d,%d,%d,%.1f,%d\n",
                cur->prescription_id, cur->record_id, cur->patient_id,
                cur->drug_id, cur->quantity, cur->price, (int)cur->status);
        cur = cur->next;
    }
    fclose(fp);
}

// 住院记录

void load_inpatients() {
    FILE *fp = fopen(FILE_INPATIENTS, "r");
    if (fp == NULL) return;

    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            continue;
        line[strcspn(line, "\r\n")] = '\0';

        InpatientNode *node = (InpatientNode *)malloc(sizeof(InpatientNode));
        if (node == NULL) continue;

        if (sscanf(line, "%d,%d,%d,%d,%d,%d,%19[^,],%19[^,],%d,%f,%d",
                   &node->inpatient_id, &node->patient_id, &node->doctor_id,
                   &node->dept_id, &node->room_id, &node->bed_num,
                   node->admit_date, node->discharge_date,
                   &node->days, &node->total_fee, &node->is_discharged) == 11) {
            node->next = NULL;
            if (inpatient_list == NULL) {
                inpatient_list = node;
            } else {
                InpatientNode *cur = inpatient_list;
                while (cur->next != NULL) cur = cur->next;
                cur->next = node;
            }
        } else {
            free(node);
        }
    }
    fclose(fp);
}

void save_inpatients() {
    FILE *fp = fopen(FILE_INPATIENTS, "w");
    if (fp == NULL) return;

    fprintf(fp, "# inpatient_id,patient_id,doctor_id,dept_id,room_id,bed_num,admit_date,discharge_date,days,total_fee,is_discharged\n");

    InpatientNode *cur = inpatient_list;
    while (cur != NULL) {
        fprintf(fp, "%d,%d,%d,%d,%d,%d,%s,%s,%d,%.1f,%d\n",
                cur->inpatient_id, cur->patient_id, cur->doctor_id,
                cur->dept_id, cur->room_id, cur->bed_num,
                cur->admit_date, cur->discharge_date,
                cur->days, cur->total_fee, cur->is_discharged);
        cur = cur->next;
    }
    fclose(fp);
}

// ID 自增生成器

int generate_medical_id() {
    int max = 20000;
    PatientNode *cur = patient_list;
    while (cur != NULL) {
        if (cur->medical_id > max) max = cur->medical_id;
        cur = cur->next;
    }
    return max + 1;
}

int generate_reg_id() {
    int max = 0;
    RegistrationNode *cur = reg_list;
    while (cur != NULL) {
        if (cur->reg_id > max) max = cur->reg_id;
        cur = cur->next;
    }
    return max + 1;
}

int generate_record_id() {
    int max = 0;
    MedicalRecordNode *cur = record_list;
    while (cur != NULL) {
        if (cur->record_id > max) max = cur->record_id;
        cur = cur->next;
    }
    return max + 1;
}

int generate_exam_order_id() {
    int max = 0;
    ExamOrderNode *cur = exam_order_list;
    while (cur != NULL) {
        if (cur->order_id > max) max = cur->order_id;
        cur = cur->next;
    }
    return max + 1;
}

int generate_prescription_id() {
    int max = 0;
    PrescriptionNode *cur = prescription_list;
    while (cur != NULL) {
        if (cur->prescription_id > max) max = cur->prescription_id;
        cur = cur->next;
    }
    return max + 1;
}

int generate_inpatient_id() {
    int max = 0;
    InpatientNode *cur = inpatient_list;
    while (cur != NULL) {
        if (cur->inpatient_id > max) max = cur->inpatient_id;
        cur = cur->next;
    }
    return max + 1;
}

// 获取指定科室当前排队人数

int get_queue_count(int dept_id) {
    int count = 0;
    RegistrationNode *cur = reg_list;
    while (cur != NULL) {
        if (cur->dept_id == dept_id && cur->status == STATUS_PENDING_PAY)
            count++;
        cur = cur->next;
    }
    return count;
}

// 统一启动加载 / 退出保存 / 释放内存

void load_all() {
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
}

void save_all() {
    save_users_to_file();
    save_drugs();
    save_rooms();
    save_registrations();
    save_medical_records();
    save_exam_orders();
    save_prescriptions();
    save_inpatients();
}

void free_all_lists() {
    StaffNode *s = staff_list;
    while (s != NULL) {
        StaffNode *tmp = s;
        s = s->next;
        free(tmp);
    }
    staff_list = NULL;

    PatientNode *p = patient_list;
    while (p != NULL) {
        PatientNode *tmp = p;
        p = p->next;
        free(tmp);
    }
    patient_list = NULL;

    DepartmentNode *d = dept_list;
    while (d != NULL) {
        DepartmentNode *tmp = d;
        d = d->next;
        free(tmp);
    }
    dept_list = NULL;

    ExamItemNode *e = exam_item_list;
    while (e != NULL) {
        ExamItemNode *tmp = e;
        e = e->next;
        free(tmp);
    }
    exam_item_list = NULL;

    DrugNode *dr = drug_list;
    while (dr != NULL) {
        DrugNode *tmp = dr;
        dr = dr->next;
        free(tmp);
    }
    drug_list = NULL;

    RoomNode *r = room_list;
    while (r != NULL) {
        RoomNode *tmp = r;
        r = r->next;
        free(tmp);
    }
    room_list = NULL;

    RegistrationNode *rg = reg_list;
    while (rg != NULL) {
        RegistrationNode *tmp = rg;
        rg = rg->next;
        free(tmp);
    }
    reg_list = NULL;

    MedicalRecordNode *mr = record_list;
    while (mr != NULL) {
        MedicalRecordNode *tmp = mr;
        mr = mr->next;
        free(tmp);
    }
    record_list = NULL;

    ExamOrderNode *eo = exam_order_list;
    while (eo != NULL) {
        ExamOrderNode *tmp = eo;
        eo = eo->next;
        free(tmp);
    }
    exam_order_list = NULL;

    PrescriptionNode *pr = prescription_list;
    while (pr != NULL) {
        PrescriptionNode *tmp = pr;
        pr = pr->next;
        free(tmp);
    }
    prescription_list = NULL;

    InpatientNode *ip = inpatient_list;
    while (ip != NULL) {
        InpatientNode *tmp = ip;
        ip = ip->next;
        free(tmp);
    }
    inpatient_list = NULL;
}