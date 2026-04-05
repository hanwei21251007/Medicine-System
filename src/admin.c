#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../include/structs.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

// 外部链表和函数声明
extern StaffNode         *staff_list;
extern PatientNode       *patient_list;
extern DepartmentNode    *dept_list;
extern DrugNode          *drug_list;
extern RoomNode          *room_list;
extern RegistrationNode  *reg_list;
extern ExamOrderNode     *exam_order_list;
extern PrescriptionNode  *prescription_list;
extern InpatientNode     *inpatient_list;
extern CurrentUser        current_user;

extern void save_all();
extern int  generate_inpatient_id();
extern const char *get_title_name(DoctorTitle title);


// ════════════════════════════════════════
// 内部工具函数
// ════════════════════════════════════════

// 彻底清空输入缓冲区，防止非法字符残留导致连锁错误
static void clear_input() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// 获取当前时间，格式 YYYY-MM-DD HH:MM:SS
static void get_today(char *buf) {
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

// 操作失败后询问是否重试，返回1=重试，返回0=返回上级菜单
static int ask_retry() {
    printf("操作失败，是否重新输入？(y=重试 / n=返回): ");
    char c;
    scanf("%c", &c);
    clear_input();
    return (c == 'y' || c == 'Y') ? 1 : 0;
}

// 根据科室ID查科室名
static const char *get_dept_name(int dept_id) {
    DepartmentNode *cur = dept_list;
    while (cur != NULL) {
        if (cur->dept_id == dept_id) return cur->dept_name;
        cur = cur->next;
    }
    return "未知科室";
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

// 根据病房ID查每日费用
static float get_room_fee(int room_id) {
    RoomNode *cur = room_list;
    while (cur != NULL) {
        if (cur->room_id == room_id) return cur->daily_fee;
        cur = cur->next;
    }
    return 0.0f;
}

// 验证姓名合法性：全中文或全英文
static int is_valid_name(const char *name) {
    int len = strlen(name);
    if (len == 0) return 0;

    int all_ascii = 1;
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)name[i];
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == ' ')) {
            all_ascii = 0;
            break;
        }
    }
    if (all_ascii) return 1;

    // 判断是否全为中文（UTF-8每个汉字3字节，首字节0xE4-0xE9）
    int i = 0;
    while (i < len) {
        unsigned char c = (unsigned char)name[i];
        if (c >= 0xE4 && c <= 0xE9) {
            if (i + 2 >= len) return 0;
            i += 3;
        } else {
            return 0;
        }
    }
    return 1;
}


// ════════════════════════════════════════
// 住院计费（每5秒自动累加一次）
// A型2元/次，B型5元/次，C型3元/次
// ════════════════════════════════════════

void billing_tick() {
    InpatientNode *cur = inpatient_list;
    int updated = 0;
    while (cur != NULL) {
        if (cur->is_discharged == 0) {
            float fee = get_room_fee(cur->room_id);
            cur->total_fee += fee;
            updated = 1;
        }
        cur = cur->next;
    }
    if (updated) save_all();
}


// ════════════════════════════════════════
// 科室信息查看
// ════════════════════════════════════════

void admin_view_departments() {
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            科室信息                    ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    printf("%-6s %-16s %-10s\n", "科室ID", "科室名称", "挂号基础价");
    printf("----------------------------------------\n");

    DepartmentNode *cur = dept_list;
    while (cur != NULL) {
        printf("%-6d %-16s %.1f元\n",
               cur->dept_id, cur->dept_name, cur->base_reg_fee);
        cur = cur->next;
    }

    printf("\n按任意键返回...");
    getchar();
}


// ════════════════════════════════════════
// 医生信息管理
// ════════════════════════════════════════

void admin_view_doctors() {
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            医生信息管理                ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    int filter_dept = -1;
    while (filter_dept < 0) {
        printf("按科室筛选？(输入科室ID，0表示显示全部): ");
        if (scanf("%d", &filter_dept) != 1 || filter_dept < 0) {
            clear_input();
            printf("输入无效。\n");
            if (!ask_retry()) return;
            filter_dept = -1;
            continue;
        }
        clear_input();
    }

    printf("\n%-8s %-10s %-16s %-12s\n", "工号", "姓名", "科室", "职称");
    printf("--------------------------------------------------\n");

    StaffNode *cur = staff_list;
    int count = 0;
    while (cur != NULL) {
        if (cur->role == ROLE_DOCTOR) {
            if (filter_dept == 0 || cur->dept_id == filter_dept) {
                printf("%-8d %-10s %-16s %-12s\n",
                       cur->id, cur->name,
                       get_dept_name(cur->dept_id),
                       get_title_name(cur->title));
                count++;
            }
        }
        cur = cur->next;
    }

    if (count == 0) printf("暂无符合条件的医生记录。\n");

    printf("\n按任意键返回...");
    getchar();
}


// ════════════════════════════════════════
// 药品信息管理
// ════════════════════════════════════════

void admin_manage_drugs() {
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            药品信息管理                ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    printf("%-6s %-20s %-10s %-8s %-8s\n",
           "药品ID", "药品名称", "单价", "库存", "预警线");
    printf("----------------------------------------------------\n");

    DrugNode *cur = drug_list;
    while (cur != NULL) {
        printf("%-6d %-20s %-10.1f %-8d %-8d\n",
               cur->drug_id, cur->drug_name,
               cur->price, cur->stock, cur->warning_line);
        cur = cur->next;
    }

    printf("\n是否修改某药品库存或预警线？(y/n): ");
    char choice;
    scanf("%c", &choice);
    clear_input();

    if (choice != 'y' && choice != 'Y') {
        printf("按任意键返回...");
        getchar();
        return;
    }

    // 查找药品，找不到可重试
    DrugNode *target = NULL;
    while (target == NULL) {
        printf("输入要修改的药品ID: ");
        int drug_id;
        if (scanf("%d", &drug_id) != 1) {
            clear_input();
            printf("输入无效。\n");
            if (!ask_retry()) return;
            continue;
        }
        clear_input();

        DrugNode *p = drug_list;
        while (p != NULL) {
            if (p->drug_id == drug_id) { target = p; break; }
            p = p->next;
        }
        if (target == NULL) {
            printf("未找到该药品。\n");
            if (!ask_retry()) return;
        }
    }

    printf("当前库存: %d，新库存 (不修改输入-1): ", target->stock);
    int new_stock;
    scanf("%d", &new_stock);
    clear_input();
    if (new_stock >= 0) target->stock = new_stock;

    printf("当前预警线: %d，新预警线 (不修改输入-1): ", target->warning_line);
    int new_warn;
    scanf("%d", &new_warn);
    clear_input();
    if (new_warn >= 0) target->warning_line = new_warn;

    save_all();
    printf("修改成功！\n");
    printf("按任意键返回...");
    getchar();
}


// ════════════════════════════════════════
// 员工信息管理
// ════════════════════════════════════════

void admin_manage_staff() {
    while (1) {
        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║            员工信息管理                ║\n");
        printf("╚════════════════════════════════════════╝\n\n");

        printf("1. 查看所有员工\n");
        printf("2. 添加新员工\n");
        printf("3. 删除员工\n");
        printf("0. 返回\n");
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

        if (choice == 0) return;

        if (choice < 1 || choice > 3) {
            printf("输入无效，请重新输入。\n");
            printf("按任意键继续...");
            getchar();
            continue;
        }

        // 查看所有员工
        if (choice == 1) {
            printf("\n%-8s %-10s %-12s %-12s %-12s\n",
                   "工号", "姓名", "角色", "科室", "职称");
            printf("------------------------------------------------------\n");
            StaffNode *cur = staff_list;
            while (cur != NULL) {
                char *role_name = "管理员";
                if (cur->role == ROLE_DOCTOR)     role_name = "医生";
                if (cur->role == ROLE_PHARMACIST) role_name = "药剂师";
                printf("%-8d %-10s %-12s %-12s %-12s\n",
                       cur->id, cur->name, role_name,
                       cur->dept_id > 0 ? get_dept_name(cur->dept_id) : "-",
                       cur->role == ROLE_DOCTOR ? get_title_name(cur->title) : "-");
                cur = cur->next;
            }
            printf("\n按任意键返回...");
            getchar();
            continue;
        }

        // 添加新员工
        if (choice == 2) {
            StaffNode *node = (StaffNode *)malloc(sizeof(StaffNode));
            if (node == NULL) {
                printf("内存分配失败。\n");
                printf("按任意键返回...");
                getchar();
                continue;
            }

            // 自动生成工号
            int max_id = 1000;
            StaffNode *check = staff_list;
            while (check != NULL) {
                if (check->id > max_id) max_id = check->id;
                check = check->next;
            }
            node->id = max_id + 1;
            printf("系统自动生成工号：%d\n", node->id);

            // 输入并验证姓名
            int name_ok = 0;
            while (!name_ok) {
                printf("输入姓名（仅限中文或英文）: ");
                fgets(node->name, sizeof(node->name), stdin);
                node->name[strcspn(node->name, "\r\n")] = '\0';
                if (strlen(node->name) == 0) {
                    printf("姓名不能为空。\n");
                    if (!ask_retry()) { free(node); break; }
                    continue;
                }
                if (is_valid_name(node->name)) {
                    name_ok = 1;
                } else {
                    printf("姓名不合法，只能全部为中文或全部为英文。\n");
                    if (!ask_retry()) { free(node); break; }
                }
            }
            if (!name_ok) continue;

            printf("输入密码: ");
            fgets(node->password, sizeof(node->password), stdin);
            node->password[strcspn(node->password, "\r\n")] = '\0';

            // 角色选择
            int role = 0;
            int role_ok = 0;
            while (!role_ok) {
                printf("选择角色 (1=管理员 2=医生 3=药剂师): ");
                if (scanf("%d", &role) != 1 || role < 1 || role > 3) {
                    clear_input();
                    printf("输入无效。\n");
                    if (!ask_retry()) { free(node); role_ok = -1; break; }
                    role = 0;
                    continue;
                }
                clear_input();
                role_ok = 1;
            }
            if (role_ok != 1) continue;

            node->role    = (UserRole)role;
            node->dept_id = 0;
            node->title   = 0;

            if (role == ROLE_DOCTOR) {
                // 科室选择
                int dept_ok = 0;
                while (!dept_ok) {
                    printf("输入所属科室ID (1-5): ");
                    if (scanf("%d", &node->dept_id) != 1 ||
                        node->dept_id < 1 || node->dept_id > 5) {
                        clear_input();
                        printf("输入无效。\n");
                        if (!ask_retry()) { free(node); dept_ok = -1; break; }
                        continue;
                    }
                    clear_input();
                    dept_ok = 1;
                }
                if (dept_ok != 1) continue;

                // 职称选择
                int title = 0;
                int title_ok = 0;
                while (!title_ok) {
                    printf("选择职称 (1=住院医师 2=主治医师 3=副主任医师 4=主任医师): ");
                    if (scanf("%d", &title) != 1 || title < 1 || title > 4) {
                        clear_input();
                        printf("输入无效。\n");
                        if (!ask_retry()) { free(node); title_ok = -1; break; }
                        title = 0;
                        continue;
                    }
                    clear_input();
                    title_ok = 1;
                }
                if (title_ok != 1) continue;
                node->title = (DoctorTitle)title;
            }

            node->next = NULL;
            if (staff_list == NULL) {
                staff_list = node;
            } else {
                StaffNode *tail = staff_list;
                while (tail->next != NULL) tail = tail->next;
                tail->next = node;
            }

            save_all();
            printf("员工添加成功！\n");
            printf("按任意键返回...");
            getchar();
            continue;
        }

        // 删除员工
        if (choice == 3) {
            StaffNode *target = NULL;
            StaffNode *prev   = NULL;

            while (target == NULL) {
                printf("输入要删除的员工工号: ");
                int del_id;
                if (scanf("%d", &del_id) != 1) {
                    clear_input();
                    printf("输入无效。\n");
                    if (!ask_retry()) break;
                    continue;
                }
                clear_input();

                if (del_id == current_user.user_id) {
                    printf("不能删除当前登录账号。\n");
                    if (!ask_retry()) break;
                    continue;
                }

                StaffNode *cur = staff_list;
                prev = NULL;
                while (cur != NULL) {
                    if (cur->id == del_id) { target = cur; break; }
                    prev = cur;
                    cur  = cur->next;
                }

                if (target == NULL) {
                    printf("未找到该工号对应的员工。\n");
                    if (!ask_retry()) break;
                }
            }

            if (target != NULL) {
                printf("确认删除员工 %s（工号%d）？(y/n): ",
                       target->name, target->id);
                char confirm;
                scanf("%c", &confirm);
                clear_input();

                if (confirm == 'y' || confirm == 'Y') {
                    if (prev == NULL) {
                        staff_list = target->next;
                    } else {
                        prev->next = target->next;
                    }
                    free(target);
                    save_all();
                    printf("删除成功！\n");
                } else {
                    printf("已取消。\n");
                }
                printf("按任意键返回...");
                getchar();
            }
            continue;
        }
    }
}


// ════════════════════════════════════════
// 统计报表
// ════════════════════════════════════════

void admin_statistics() {
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              统计报表                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 各科室挂号人数
    printf("【各科室挂号人数】\n");
    printf("%-16s %-10s\n", "科室名称", "挂号人数");
    printf("----------------------------\n");
    DepartmentNode *dept = dept_list;
    while (dept != NULL) {
        int count = 0;
        RegistrationNode *reg = reg_list;
        while (reg != NULL) {
            if (reg->dept_id == dept->dept_id) count++;
            reg = reg->next;
        }
        printf("%-16s %-10d\n", dept->dept_name, count);
        dept = dept->next;
    }

    // 当前在院患者数量
    printf("\n【当前在院患者】\n");
    int inpatient_count = 0;
    InpatientNode *ip = inpatient_list;
    while (ip != NULL) {
        if (ip->is_discharged == 0) inpatient_count++;
        ip = ip->next;
    }
    printf("当前在院患者总数：%d 人\n", inpatient_count);

    // 病房使用情况
    printf("\n【病房使用情况】\n");
    printf("%-8s %-16s %-10s %-8s %-8s\n",
           "病房ID", "科室", "类型", "总床位", "已占用");
    printf("----------------------------------------------------\n");
    RoomNode *room = room_list;
    while (room != NULL) {
        char *type_name = "普通多人间";
        if (room->room_type == ROOM_B) type_name = "术后观察";
        if (room->room_type == ROOM_C) type_name = "急诊留观";
        printf("%-8d %-16s %-10s %-8d %-8d\n",
               room->room_id, get_dept_name(room->dept_id),
               type_name, room->capacity, room->current);
        room = room->next;
    }

    // 药品库存预警
    printf("\n【药品库存预警】\n");
    int warn_count = 0;
    DrugNode *drug = drug_list;
    while (drug != NULL) {
        if (drug->stock <= drug->warning_line) {
            if (warn_count == 0) {
                printf("%-6s %-20s %-8s %-8s\n",
                       "药品ID", "药品名称", "当前库存", "预警线");
                printf("------------------------------------------\n");
            }
            printf("%-6d %-20s %-8d %-8d\n",
                   drug->drug_id, drug->drug_name,
                   drug->stock, drug->warning_line);
            warn_count++;
        }
        drug = drug->next;
    }
    if (warn_count == 0) printf("当前无库存预警药品。\n");

    // 医院收入统计
    printf("\n【医院收入统计】\n");

    float reg_income = 0.0f;
    RegistrationNode *reg = reg_list;
    while (reg != NULL) {
        if (reg->status == STATUS_PENDING_DO || reg->status == STATUS_DONE)
            reg_income += reg->reg_fee;
        reg = reg->next;
    }

    float exam_income = 0.0f;
    ExamOrderNode *exam = exam_order_list;
    while (exam != NULL) {
        if (exam->status == STATUS_PENDING_DO || exam->status == STATUS_DONE)
            exam_income += exam->price;
        exam = exam->next;
    }

    float drug_income = 0.0f;
    PrescriptionNode *presc = prescription_list;
    while (presc != NULL) {
        if (presc->status == STATUS_PENDING_DO || presc->status == STATUS_DONE)
            drug_income += presc->price;
        presc = presc->next;
    }

    float inpatient_income = 0.0f;
    InpatientNode *inp = inpatient_list;
    while (inp != NULL) {
        inpatient_income += inp->total_fee;
        inp = inp->next;
    }

    printf("挂号收入：    %.1f 元\n", reg_income);
    printf("检查收入：    %.1f 元\n", exam_income);
    printf("药品收入：    %.1f 元\n", drug_income);
    printf("住院收入：    %.1f 元\n", inpatient_income);
    printf("---------------------------\n");
    printf("总收入：      %.1f 元\n",
           reg_income + exam_income + drug_income + inpatient_income);

    printf("\n按任意键返回...");
    getchar();
}


// ════════════════════════════════════════
// 住院登记
// ════════════════════════════════════════

void admin_admit_patient() {
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              住院登记                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 查找患者，找不到可重试
    PatientNode *patient = NULL;
    int medical_id = 0;
    while (patient == NULL) {
        printf("输入患者病历号: ");
        if (scanf("%d", &medical_id) != 1) {
            clear_input();
            printf("输入无效。\n");
            if (!ask_retry()) return;
            continue;
        }
        clear_input();

        PatientNode *p = patient_list;
        while (p != NULL) {
            if (p->medical_id == medical_id) { patient = p; break; }
            p = p->next;
        }
        if (patient == NULL) {
            printf("未找到该病历号对应的患者。\n");
            if (!ask_retry()) return;
        }
    }

    // 检查是否已在院
    InpatientNode *check = inpatient_list;
    while (check != NULL) {
        if (check->patient_id == medical_id && check->is_discharged == 0) {
            printf("该患者当前已在院，无需重复登记。\n");
            printf("按任意键返回...");
            getchar();
            return;
        }
        check = check->next;
    }

    printf("患者姓名：%s\n\n", patient->name);

    // 查找医生，找不到可重试
    StaffNode *doctor = NULL;
    int doctor_id = 0;
    while (doctor == NULL) {
        printf("输入主治医生工号: ");
        if (scanf("%d", &doctor_id) != 1) {
            clear_input();
            printf("输入无效。\n");
            if (!ask_retry()) return;
            continue;
        }
        clear_input();

        StaffNode *s = staff_list;
        while (s != NULL) {
            if (s->id == doctor_id && s->role == ROLE_DOCTOR) { doctor = s; break; }
            s = s->next;
        }
        if (doctor == NULL) {
            printf("未找到该工号对应的医生。\n");
            if (!ask_retry()) return;
        }
    }

    int dept_id = doctor->dept_id;
    printf("科室：%s\n\n", get_dept_name(dept_id));

    // 优先选已有病人未满的病房
    RoomNode *best_room = NULL;
    RoomNode *cur = room_list;
    while (cur != NULL) {
        if (cur->dept_id == dept_id && cur->current < cur->capacity) {
            if (best_room == NULL) {
                best_room = cur;
            } else if (cur->current > best_room->current) {
                best_room = cur;
            }
        }
        cur = cur->next;
    }

    if (best_room == NULL) {
        printf("该科室暂无空余床位，无法办理住院。\n");
        printf("按任意键返回...");
        getchar();
        return;
    }

    int bed_num = best_room->current + 1;
    char *type_name = "普通多人间";
    if (best_room->room_type == ROOM_B) type_name = "术后观察";
    if (best_room->room_type == ROOM_C) type_name = "急诊留观";

    printf("分配病房：%d号房（%s），床位：%d号，计费单位：%.1f元/5秒\n\n",
           best_room->room_id, type_name, bed_num, best_room->daily_fee);

    printf("确认办理住院？(y/n): ");
    char choice;
    scanf("%c", &choice);
    clear_input();

    if (choice != 'y' && choice != 'Y') {
        printf("已取消。\n");
        printf("按任意键返回...");
        getchar();
        return;
    }

    InpatientNode *node = (InpatientNode *)malloc(sizeof(InpatientNode));
    if (node == NULL) {
        printf("内存分配失败，住院登记取消。\n");
        printf("按任意键返回...");
        getchar();
        return;
    }

    node->inpatient_id  = generate_inpatient_id();
    node->patient_id    = medical_id;
    node->doctor_id     = doctor_id;
    node->dept_id       = dept_id;
    node->room_id       = best_room->room_id;
    node->bed_num       = bed_num;
    node->days          = 0;
    node->total_fee     = 0.0f;
    node->is_discharged = 0;
    get_today(node->admit_date);
    strcpy(node->discharge_date, "-");
    node->next = NULL;

    if (inpatient_list == NULL) {
        inpatient_list = node;
    } else {
        InpatientNode *tail = inpatient_list;
        while (tail->next != NULL) tail = tail->next;
        tail->next = node;
    }

    best_room->current += 1;
    save_all();

    printf("\n住院登记成功！住院ID：%d，入院时间：%s\n",
           node->inpatient_id, node->admit_date);
    printf("系统每5秒自动计费一次。\n");
    printf("按任意键返回...");
    getchar();
}


// ════════════════════════════════════════
// 在院患者列表
// ════════════════════════════════════════

void admin_view_inpatients() {
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║            在院患者列表                ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    printf("%-8s %-10s %-16s %-6s %-6s %-22s %-10s\n",
           "住院ID", "姓名", "科室", "病房", "床位", "入院时间", "已累计费用");
    printf("------------------------------------------------------------------------------\n");

    InpatientNode *cur = inpatient_list;
    int count = 0;
    while (cur != NULL) {
        if (cur->is_discharged == 0) {
            printf("%-8d %-10s %-16s %-6d %-6d %-22s %.1f元\n",
                   cur->inpatient_id,
                   get_patient_name(cur->patient_id),
                   get_dept_name(cur->dept_id),
                   cur->room_id,
                   cur->bed_num,
                   cur->admit_date,
                   cur->total_fee);
            count++;
        }
        cur = cur->next;
    }

    if (count == 0) printf("当前无在院患者。\n");

    printf("\n按任意键返回...");
    getchar();
}


// ════════════════════════════════════════
// 出院登记
// ════════════════════════════════════════

void admin_discharge_patient() {
    system("cls");
    printf("╔════════════════════════════════════════╗\n");
    printf("║              出院登记                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");

    // 查找住院记录，找不到可重试
    InpatientNode *target = NULL;
    while (target == NULL) {
        printf("输入住院ID: ");
        int inpatient_id;
        if (scanf("%d", &inpatient_id) != 1) {
            clear_input();
            printf("输入无效。\n");
            if (!ask_retry()) return;
            continue;
        }
        clear_input();

        InpatientNode *p = inpatient_list;
        while (p != NULL) {
            if (p->inpatient_id == inpatient_id) { target = p; break; }
            p = p->next;
        }
        if (target == NULL) {
            printf("未找到该住院记录。\n");
            if (!ask_retry()) return;
        }
    }

    if (target->is_discharged == 1) {
        printf("该患者已办理出院，无需重复操作。\n");
        printf("按任意键返回...");
        getchar();
        return;
    }

    char today[25];
    get_today(today);

    printf("患者：%s\n", get_patient_name(target->patient_id));
    printf("入院时间：%s\n", target->admit_date);
    printf("出院时间：%s\n", today);
    printf("住院累计费用：%.1f 元\n\n", target->total_fee);

    printf("确认办理出院？(y/n): ");
    char choice;
    scanf("%c", &choice);
    clear_input();

    if (choice != 'y' && choice != 'Y') {
        printf("已取消。\n");
        printf("按任意键返回...");
        getchar();
        return;
    }

    strcpy(target->discharge_date, today);
    target->is_discharged = 1;

    RoomNode *room = room_list;
    while (room != NULL) {
        if (room->room_id == target->room_id) {
            if (room->current > 0) room->current -= 1;
            break;
        }
        room = room->next;
    }

    save_all();

    printf("\n出院登记成功！结算费用：%.1f 元\n", target->total_fee);
    printf("按任意键返回...");
    getchar();
}


// ════════════════════════════════════════
// 管理员主菜单（含5秒自动计费）
// ════════════════════════════════════════

void admin_menu() {
    time_t last_billing = time(NULL);

    while (1) {
        time_t now = time(NULL);
        if (now - last_billing >= 5) {
            billing_tick();
            last_billing = now;
        }

        system("cls");
        printf("╔════════════════════════════════════════╗\n");
        printf("║  欢迎，%-16s              ║\n", current_user.user_name);
        printf("╠════════════════════════════════════════╣\n");
        printf("║  1. 科室信息查看                       ║\n");
        printf("║  2. 医生信息管理                       ║\n");
        printf("║  3. 药品信息管理                       ║\n");
        printf("║  4. 统计报表                           ║\n");
        printf("║  5. 员工信息管理                       ║\n");
        printf("║  6. 住院登记                           ║\n");
        printf("║  7. 在院患者列表                       ║\n");
        printf("║  8. 出院登记                           ║\n");
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

        if (choice < 0 || choice > 8) {
            printf("输入无效，请重新输入。\n");
            printf("按任意键继续...");
            getchar();
            continue;
        }

        switch (choice) {
            case 1: admin_view_departments();  break;
            case 2: admin_view_doctors();      break;
            case 3: admin_manage_drugs();      break;
            case 4: admin_statistics();        break;
            case 5: admin_manage_staff();      break;
            case 6: admin_admit_patient();     break;
            case 7: admin_view_inpatients();   break;
            case 8: admin_discharge_patient(); break;
        }
    }
}