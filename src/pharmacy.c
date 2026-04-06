#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/structs.h"

// 外部链表和函数声明
extern PatientNode       *patient_list;
extern DrugNode          *drug_list;
extern PrescriptionNode  *prescription_list;
extern CurrentUser        current_user;

extern void save_all();


// ════════════════════════════════════════
// 内部工具函数
// ════════════════════════════════════════

// 彻底清空输入缓冲区
static void clear_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// 操作失败后询问是否重试
static int ask_retry() {
    printf("操作失败，是否重新输入？(y=重试 / n=返回): ");
    char c;
    scanf("%c", &c);
    clear_input();
    return (c == 'y' || c == 'Y') ? 1 : 0;
}

// 根据病历号查患者姓名
static const char *get_patient_name(int medical_id) {
    PatientNode *cur = patient_list;
    while (cur != NULL) {
        if (cur->medical_id == medical_id) return cur->name;
        cur = cur->next;
    }
    return "未知患者";
}

// 根据药品ID查药品名
static const char *get_drug_name(int drug_id) {
    DrugNode *cur = drug_list;
    while (cur != NULL) {
        if (cur->drug_id == drug_id) return cur->drug_name;
        cur = cur->next;
    }
    return "未知药品";
}


// ════════════════════════════════════════
// 待发药处方列表
// ════════════════════════════════════════

void pharmacy_view_pending() {
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║          待发药处方列表                ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    printf("%-8s %-10s %-20s %-6s %-8s\n",
           "处方ID", "患者姓名", "药品名称", "数量", "总价");
    printf("----------------------------------------------------\n");

    PrescriptionNode *cur = prescription_list;
    int count = 0;
    while (cur != NULL) {
        if (cur->status == STATUS_PENDING_DO) {
            printf("%-8d %-10s %-20s %-6d %.1f元\n",
                   cur->prescription_id,
                   get_patient_name(cur->patient_id),
                   get_drug_name(cur->drug_id),
                   cur->quantity,
                   cur->price);
            count++;
        }
        cur = cur->next;
    }

    if (count == 0) printf("当前无待发药处方。\n");

    printf("\n按任意键返回...");
    getchar();
}


// ════════════════════════════════════════
// 发药操作
// ════════════════════════════════════════

void pharmacy_dispense() {
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              发药操作                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 先显示待发药列表
    printf("【待发药处方】\n");
    printf("%-8s %-10s %-20s %-6s %-8s\n",
           "处方ID", "患者姓名", "药品名称", "数量", "总价");
    printf("----------------------------------------------------\n");

    PrescriptionNode *cur = prescription_list;
    int count = 0;
    while (cur != NULL) {
        if (cur->status == STATUS_PENDING_DO) {
            printf("%-8d %-10s %-20s %-6d %.1f元\n",
                   cur->prescription_id,
                   get_patient_name(cur->patient_id),
                   get_drug_name(cur->drug_id),
                   cur->quantity,
                   cur->price);
            count++;
        }
        cur = cur->next;
    }

    if (count == 0) {
        printf("当前无待发药处方。\n");
        printf("按任意键返回...");
        getchar();
        return;
    }

    // 选择要发药的处方
    PrescriptionNode *target = NULL;
    while (target == NULL) {
        printf("\n输入要发药的处方ID (0=取消): ");
        int presc_id;
        if (scanf("%d", &presc_id) != 1) {
            clear_input();
            printf("输入无效。\n");
            if (!ask_retry()) return;
            continue;
        }
        clear_input();
        if (presc_id == 0) return;

        PrescriptionNode *p = prescription_list;
        while (p != NULL) {
            if (p->prescription_id == presc_id &&
                p->status == STATUS_PENDING_DO) {
                target = p;
                break;
            }
            p = p->next;
        }
        if (target == NULL) {
            printf("未找到该处方，或该处方不在待发药状态。\n");
            if (!ask_retry()) return;
        }
    }

    // 查找对应药品
    DrugNode *drug = drug_list;
    while (drug != NULL) {
        if (drug->drug_id == target->drug_id) break;
        drug = drug->next;
    }

    if (drug == NULL) {
        printf("未找到对应药品信息，无法发药。\n");
        printf("按任意键返回...");
        getchar();
        return;
    }

    // 检查库存
    if (drug->stock < target->quantity) {
        printf("库存不足！当前库存 %d 盒，处方需要 %d 盒，无法发药。\n",
               drug->stock, target->quantity);
        printf("请先补充库存。\n");
        printf("按任意键返回...");
        getchar();
        return;
    }

    // 显示发药信息确认
    printf("\n发药确认：\n");
    printf("  患者：%s\n",       get_patient_name(target->patient_id));
    printf("  药品：%s\n",       drug->drug_name);
    printf("  数量：%d 盒\n",    target->quantity);
    printf("  总价：%.1f 元\n",  target->price);
    printf("  发药后库存：%d 盒\n", drug->stock - target->quantity);

    printf("\n确认发药？(y/n): ");
    char confirm;
    scanf("%c", &confirm);
    clear_input();

    if (confirm != 'y' && confirm != 'Y') {
        printf("已取消。\n");
        printf("按任意键返回...");
        getchar();
        return;
    }

    // 扣减库存，更新处方状态
    drug->stock     -= target->quantity;
    target->status   = STATUS_DONE;
    save_all();

    printf("\n发药成功！\n");
    printf("  %s 剩余库存：%d 盒\n", drug->drug_name, drug->stock);

    // 库存预警提示
    if (drug->stock <= drug->warning_line) {
        printf("  库存预警：%s 库存已低于预警线（%d盒），请及时补货！\n",
               drug->drug_name, drug->warning_line);
    }

    printf("按任意键返回...");
    getchar();
}


// ════════════════════════════════════════
// 药品库存查询
// ════════════════════════════════════════

void pharmacy_view_stock() {
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            药品库存查询                ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    printf("%-6s %-20s %-8s %-8s %-8s\n",
           "药品ID", "药品名称", "单价", "库存", "预警线");
    printf("----------------------------------------------------\n");

    DrugNode *cur = drug_list;
    int count = 0;
    while (cur != NULL) {
        // 库存低于预警线时加标注
        if (cur->stock <= cur->warning_line) {
            printf("%-6d %-20s %-8.1f %-8d %-8d [库存不足]\n",
                   cur->drug_id, cur->drug_name,
                   cur->price, cur->stock, cur->warning_line);
        } else {
            printf("%-6d %-20s %-8.1f %-8d %-8d\n",
                   cur->drug_id, cur->drug_name,
                   cur->price, cur->stock, cur->warning_line);
        }
        count++;
        cur = cur->next;
    }

    if (count == 0) printf("暂无药品信息。\n");

    printf("\n按任意键返回...");
    getchar();
}


// ════════════════════════════════════════
// 库存预警列表
// ════════════════════════════════════════

void pharmacy_view_warning() {
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            库存预警                    ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    printf("%-6s %-20s %-8s %-8s\n",
           "药品ID", "药品名称", "当前库存", "预警线");
    printf("------------------------------------------\n");

    DrugNode *cur = drug_list;
    int count = 0;
    while (cur != NULL) {
        if (cur->stock <= cur->warning_line) {
            printf("%-6d %-20s %-8d %-8d\n",
                   cur->drug_id, cur->drug_name,
                   cur->stock, cur->warning_line);
            count++;
        }
        cur = cur->next;
    }

    if (count == 0) printf("当前无库存预警药品，库存充足。\n");

    printf("\n按任意键返回...");
    getchar();
}


// ════════════════════════════════════════
// 药剂师主菜单
// ════════════════════════════════════════

void pharmacy_menu() {
    while (1) {
        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║  欢迎，%-16s              ║\n", current_user.user_name);
        printf("╠════════════════════════════════════════╣\n");
        printf("║  1. 待发药处方列表                     ║\n");
        printf("║  2. 发药操作                           ║\n");
        printf("║  3. 药品库存查询                       ║\n");
        printf("║  4. 库存预警                           ║\n");
        printf("║  0. 注销登录                           ║\n");
        printf("╚════════════════════════════════════════╝\n");
        printf("\n请输入您的选择: ");

        int choice;
        if (scanf("%d", &choice) != 1) {
            clear_input();
            printf("输入无效，请重新输入。\n");
            printf("按任意键继续...");
            getchar();
            continue;
        }
        clear_input();

        if (choice == 0) break;

        if (choice < 1 || choice > 4) {
            printf("输入无效，请重新输入。\n");
            printf("按任意键继续...");
            getchar();
            continue;
        }

        switch (choice) {
            case 1: pharmacy_view_pending(); break;
            case 2: pharmacy_dispense();     break;
            case 3: pharmacy_view_stock();   break;
            case 4: pharmacy_view_warning(); break;
        }
    }
}