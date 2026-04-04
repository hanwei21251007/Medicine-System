#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/structs.h"

#define USERS_FILE "data/users.txt"

StaffNode   *staff_list   = NULL;
PatientNode *patient_list = NULL;


//内部工具：链表尾插

static void append_staff(int id, const char *pwd, const char *name, UserRole role) {
    StaffNode *node = (StaffNode *)malloc(sizeof(StaffNode));
    if (!node) return;
    node->id   = id;
    node->role = role;
    strncpy(node->password, pwd,  sizeof(node->password) - 1);
    strncpy(node->name,     name, sizeof(node->name)     - 1);
    node->password[sizeof(node->password) - 1] = '\0';
    node->name[sizeof(node->name) - 1]         = '\0';
    node->next = NULL;

    if (!staff_list) { staff_list = node; return; }
    StaffNode *cur = staff_list;
    while (cur->next) cur = cur->next;
    cur->next = node;
}

static void append_patient(int mid, const char *id_card,const char *pwd, const char *name) {
    PatientNode *node = (PatientNode *)malloc(sizeof(PatientNode));
    if (!node) return;
    node->medical_id = mid;
    strncpy(node->id_card,  id_card, sizeof(node->id_card)  - 1);
    strncpy(node->password, pwd,     sizeof(node->password) - 1);
    strncpy(node->name,     name,    sizeof(node->name)     - 1);
    node->id_card[sizeof(node->id_card)   - 1] = '\0';
    node->password[sizeof(node->password) - 1] = '\0';
    node->name[sizeof(node->name)         - 1] = '\0';
    node->next = NULL;

    if (!patient_list) { patient_list = node; return; }
    PatientNode *cur = patient_list;
    while (cur->next) cur = cur->next;
    cur->next = node;
}


//读取文件 → 链表

void load_users_from_file() {
    FILE *fp = fopen(USERS_FILE, "r");
    if (!fp) {
        printf("[警告] 未找到 %s，将以空数据启动。\n", USERS_FILE);
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), fp)) {
        //跳过注释和空行
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        //去掉行尾换行 
        line[strcspn(line, "\r\n")] = '\0';

        char type[16], name[50], pwd[50], id_card[19];
        int  id, role;

        //尝试解析 STAFF 
        if (sscanf(line, "STAFF,%d,%49[^,],%49[^,],%d,",
                   &id, pwd, name, &role) == 4) {
            append_staff(id, pwd, name, (UserRole)role);
            continue;
        }

        //尝试解析 PATIENT 
        if (sscanf(line, "PATIENT,%d,%49[^,],%49[^,],%d,%18s",
                   &id, pwd, name, &role, id_card) == 5) {
            append_patient(id, id_card, pwd, name);
            continue;
        }
    }

    fclose(fp);
}


//写回文件 ← 链表

void save_users_to_file() {
    FILE *fp = fopen(USERS_FILE, "w");
    if (!fp) {
        printf("[错误] 无法写入 %s，数据未保存！\n", USERS_FILE);
        return;
    }

    fprintf(fp, "# type,id/medical_id,password,name,role,id_card\n");

    StaffNode *s = staff_list;
    while (s) {
        fprintf(fp, "STAFF,%d,%s,%s,%d,\n",
                s->id, s->password, s->name, (int)s->role);
        s = s->next;
    }

    PatientNode *p = patient_list;
    while (p) {
        fprintf(fp, "PATIENT,%d,%s,%s,4,%s\n",
                p->medical_id, p->password, p->name, p->id_card);
        p = p->next;
    }

    fclose(fp);
}


//生成下一个病历号

int generate_medical_id() {
    int max_id = 20000;
    PatientNode *cur = patient_list;
    while (cur) {
        if (cur->medical_id > max_id) max_id = cur->medical_id;
        cur = cur->next;
    }
    return max_id + 1;
}


//释放链表内存（程序退出时调用）

void free_all_lists() {
    StaffNode *s = staff_list;
    while (s) { StaffNode *tmp = s; s = s->next; free(tmp); }
    staff_list = NULL;

    PatientNode *p = patient_list;
    while (p) { PatientNode *tmp = p; p = p->next; free(tmp); }
    patient_list = NULL;
}