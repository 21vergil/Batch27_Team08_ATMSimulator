#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#define BOX_WIDTH 49
#define MAX_ACCOUNTS 20

typedef struct {
    char last4[5];
    char name[32];
    char pin[5];
    double balance;
    int is_blocked;
    int attempts;
} Account;

static Account accounts[MAX_ACCOUNTS];
static int total_accounts = 0;

static void load_json(const char *path) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        printf("Error: Could not open %s\n", path);
        exit(1);
    }

    char line[256];
    Account acc;
    memset(&acc, 0, sizeof(Account));
    int inside_obj = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (strstr(line, "{") && !strstr(line, "accounts")) {
            memset(&acc, 0, sizeof(Account));
            inside_obj = 1;
        }

        char *p;
        if ((p = strstr(line, "\"last4\":")))   sscanf(p, "\"last4\": \"%4[^\"]\"", acc.last4);
        if ((p = strstr(line, "\"name\":")))    sscanf(p, "\"name\": \"%31[^\"]\"", acc.name);
        if ((p = strstr(line, "\"pin\":")))     sscanf(p, "\"pin\": \"%4[^\"]\"", acc.pin);
        if ((p = strstr(line, "\"balance\":"))) sscanf(p, "\"balance\": %lf", &acc.balance);
        if ((p = strstr(line, "\"is_blocked\":"))) acc.is_blocked = (strstr(line, "true") != NULL);

        if (strstr(line, "}") && inside_obj) {
            if (strlen(acc.last4) == 4 && total_accounts < MAX_ACCOUNTS) {
                accounts[total_accounts++] = acc;
            }
            inside_obj = 0;
        }
    }
    fclose(fp);
}

static void save_json(const char *path) {
    FILE *fp = fopen(path, "w");
    if (!fp) return;

    fprintf(fp, "{\n  \"accounts\": [\n");
    for (int i = 0; i < total_accounts; i++) {
        fprintf(fp, "    {\n");
        fprintf(fp, "      \"last4\": \"%s\",\n", accounts[i].last4);
        fprintf(fp, "      \"name\": \"%s\",\n", accounts[i].name);
        fprintf(fp, "      \"pin\": \"%s\",\n", accounts[i].pin);
        fprintf(fp, "      \"balance\": %.2f,\n", accounts[i].balance);
        fprintf(fp, "      \"is_blocked\": %s,\n", accounts[i].is_blocked ? "true" : "false");
        fprintf(fp, "      \"failed_attempts\": %d\n", accounts[i].attempts);
        fprintf(fp, "    }%s\n", (i == total_accounts - 1) ? "" : ",");
    }
    fprintf(fp, "  ]\n}\n");
    fclose(fp);
}

static int cmp_acc(const void *a, const void *b) {
    return strcmp(((Account *)a)->last4, ((Account *)b)->last4);
}

static void sort_accounts(void) {
    qsort(accounts, total_accounts, sizeof(Account), cmp_acc);
}

static int binary_search_card(const char *last4) {
    int low = 0, high = total_accounts - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        int cmp = strcmp(accounts[mid].last4, last4);
        if (cmp == 0) return mid;
        if (cmp < 0)  low = mid + 1;
        else          high = mid - 1;
    }
    return -1;
}

#define KEY_UP    1001
#define KEY_DOWN  1002
#define KEY_ENTER 1003
#define KEY_ESC   1004

static struct termios orig_term;

static void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_term);
}

static void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_term);
    atexit(disable_raw_mode);
    struct termios raw = orig_term;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static int read_key(void) {
    char c;
    if (read(STDIN_FILENO, &c, 1) != 1) return 0;
    if (c == '\n' || c == '\r') return KEY_ENTER;
    if (c == 27) {
        char seq[2];
        if (read(STDIN_FILENO, &seq[0], 1) == 1 && read(STDIN_FILENO, &seq[1], 1) == 1) {
            if (seq[0] == '[' && seq[1] == 'A') return KEY_UP;
            if (seq[0] == '[' && seq[1] == 'B') return KEY_DOWN;
        }
        return KEY_ESC;
    }
    return c;
}

static int display_width(const char *s) {
    int width = 0;
    while (*s) {
        if ((*s & 0xC0) != 0x80) width++;
        s++;
    }
    return width;
}

static void draw_border(void) {
    for (int i = 0; i < BOX_WIDTH; i++) putchar('*');
    putchar('\n');
}

static void draw_sep(void) {
    putchar('*');
    for (int i = 0; i < BOX_WIDTH - 2; i++) putchar('-');
    putchar('*');
    putchar('\n');
}

static void draw_line(const char *text) {
    int len = display_width(text);
    int space = BOX_WIDTH - 2 - len;
    if (space < 0) space = 0;
    printf("* %s", text);
    for (int i = 0; i < space - 1; i++) putchar(' ');
    printf("*\n");
}

static void draw_center(const char *text) {
    int len = display_width(text);
    int pad = (BOX_WIDTH - 2 - len) / 2;
    int rem = BOX_WIDTH - 2 - len - pad;
    if (pad < 0) pad = 0;
    if (rem < 0) rem = 0;
    putchar('*');
    for (int i = 0; i < pad; i++) putchar(' ');
    printf("%s", text);
    for (int i = 0; i < rem; i++) putchar(' ');
    printf("*\n");
}

static int show_menu(const char *title, const char *subtitle, const char **items, int count) {
    int sel = 0;
    while (1) {
        printf("\033[2J\033[H");
        draw_border();
        draw_center("A T M");
        draw_center(title);
        if (subtitle) draw_center(subtitle);
        draw_sep();
        draw_line("Use [UP/DOWN] Arrow Keys & [ENTER]");
        draw_line("");

        for (int i = 0; i < count; i++) {
            char buf[64];
            if (i == sel) snprintf(buf, sizeof(buf), ">> [%d] %s <<", i + 1, items[i]);
            else         snprintf(buf, sizeof(buf), "   [%d] %s", i + 1, items[i]);
            draw_line(buf);
        }

        draw_line("");
        draw_sep();
        draw_line("[ENTER] Select  |  [ESC] Exit");
        draw_border();

        int k = read_key();
        if (k == KEY_UP)    sel = (sel - 1 + count) % count;
        if (k == KEY_DOWN)  sel = (sel + 1) % count;
        if (k == KEY_ENTER) return sel;
        if (k == KEY_ESC)   return -1;
    }
}

static void get_4digits(const char *prompt, char *dest, int masked) {
    int pos = 0;
    dest[0] = '\0';
    while (1) {
        printf("\033[2J\033[H");
        draw_border();
        draw_center("A T M");
        draw_sep();
        draw_line("");
        draw_center(prompt);
        draw_line("");

        char display[64];
        int off = snprintf(display, sizeof(display), "Input: [ ");
        for (int i = 0; i < 4; i++) {
            if (i < pos) off += snprintf(display + off, sizeof(display) - off, "%c ", masked ? '*' : dest[i]);
            else         off += snprintf(display + off, sizeof(display) - off, "_ ");
        }
        snprintf(display + off, sizeof(display) - off, "]");
        draw_center(display);

        draw_line("");
        draw_sep();
        draw_line("Type 4 digits | [BACKSPACE] Del | [ESC] Cancel");
        draw_border();

        int k = read_key();
        if (k == KEY_ESC) { dest[0] = '\0'; return; }
        if (k == 127 || k == 8) {
            if (pos > 0) { pos--; dest[pos] = '\0'; }
        } else if (k >= '0' && k <= '9' && pos < 4) {
            dest[pos++] = (char)k;
            dest[pos] = '\0';
            if (pos == 4) return;
        }
    }
}

static int get_amount(const char *prompt, double *amt) {
    char buf[32] = "";
    int pos = 0;
    int has_dot = 0;

    while (1) {
        printf("\033[2J\033[H");
        draw_border();
        draw_center("A T M");
        draw_sep();
        draw_line("");
        draw_center(prompt);
        draw_line("");

        char display[64];
        snprintf(display, sizeof(display), "Amount: [ ₹ %s%s ]", buf, (pos == 0) ? "_" : "");
        draw_center(display);

        draw_line("");
        draw_sep();
        draw_line("Type amount | [ENTER] Confirm | [ESC] Cancel");
        draw_border();

        int k = read_key();
        if (k == KEY_ESC) return 0;
        if (k == KEY_ENTER) {
            if (pos > 0) {
                *amt = atof(buf);
                if (*amt > 0) return 1;
            }
        } else if (k == 127 || k == 8) {
            if (pos > 0) {
                pos--;
                if (buf[pos] == '.') has_dot = 0;
                buf[pos] = '\0';
            }
        } else if (k >= '0' && k <= '9' && pos < 12) {
            buf[pos++] = (char)k;
            buf[pos] = '\0';
        } else if (k == '.' && !has_dot && pos < 12) {
            buf[pos++] = '.';
            buf[pos] = '\0';
            has_dot = 1;
        }
    }
}

static void show_message(const char *title, const char *msg) {
    printf("\033[2J\033[H");
    draw_border();
    draw_center("A T M");
    draw_sep();
    draw_line("");
    draw_center(title);
    draw_line("");
    draw_center(msg);
    draw_line("");
    draw_sep();
    draw_line("Press any key to continue...");
    draw_border();
    read_key();
}

static void write_log(const char *last4, const char *action, const char *details) {
    mkdir("logs", 0755);
    FILE *fp = fopen("logs/atm.log", "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    fprintf(fp, "[%s] CARD:****%s | %-12s | %s\n", time_str, last4 ? last4 : "----", action, details);
    fclose(fp);
}

static unsigned long crc_table[256];
static int crc_ready = 0;

static void crc_init(void) {
    for (unsigned int n = 0; n < 256; n++) {
        unsigned long c = n;
        for (int k = 0; k < 8; k++) c = (c & 1) ? (0xEDB88320UL ^ (c >> 1)) : (c >> 1);
        crc_table[n] = c;
    }
    crc_ready = 1;
}

static unsigned long crc_update(unsigned long crc, const unsigned char *buf, size_t len) {
    if (!crc_ready) crc_init();
    for (size_t i = 0; i < len; i++) crc = crc_table[(crc ^ buf[i]) & 0xFF] ^ (crc >> 8);
    return crc;
}

static void write_chunk(FILE *fp, const char *type, const unsigned char *data, size_t len) {
    unsigned char len_b[4] = { (len >> 24) & 0xFF, (len >> 16) & 0xFF, (len >> 8) & 0xFF, len & 0xFF };
    fwrite(len_b, 1, 4, fp);
    fwrite(type, 1, 4, fp);
    if (len > 0) fwrite(data, 1, len, fp);

    unsigned long crc = crc_update(0xFFFFFFFFUL, (const unsigned char *)type, 4);
    if (len > 0) crc = crc_update(crc, data, len);
    crc ^= 0xFFFFFFFFUL;

    unsigned char crc_b[4] = { (crc >> 24) & 0xFF, (crc >> 16) & 0xFF, (crc >> 8) & 0xFF, crc & 0xFF };
    fwrite(crc_b, 1, 4, fp);
}

static const unsigned char FONT[39][5] = {
    {0x3E,0x51,0x49,0x45,0x3E},{0x00,0x42,0x7F,0x40,0x00},{0x42,0x61,0x51,0x49,0x46},{0x21,0x41,0x45,0x4B,0x31},
    {0x18,0x14,0x12,0x7F,0x10},{0x27,0x45,0x45,0x45,0x39},{0x3C,0x4A,0x49,0x49,0x30},{0x01,0x71,0x09,0x05,0x03},
    {0x36,0x49,0x49,0x49,0x36},{0x06,0x49,0x49,0x29,0x1E},{0x7E,0x11,0x11,0x11,0x7E},{0x7F,0x49,0x49,0x49,0x36},
    {0x3E,0x41,0x41,0x41,0x22},{0x7F,0x41,0x41,0x22,0x1C},{0x7F,0x49,0x49,0x49,0x41},{0x7F,0x09,0x09,0x09,0x01},
    {0x3E,0x41,0x49,0x49,0x7A},{0x7F,0x08,0x08,0x08,0x7F},{0x00,0x41,0x7F,0x41,0x00},{0x20,0x40,0x41,0x3F,0x01},
    {0x7F,0x08,0x14,0x22,0x41},{0x7F,0x40,0x40,0x40,0x40},{0x7F,0x02,0x0C,0x02,0x7F},{0x7F,0x04,0x08,0x10,0x7F},
    {0x3E,0x41,0x41,0x41,0x3E},{0x7F,0x09,0x09,0x09,0x06},{0x3E,0x41,0x51,0x21,0x5E},{0x7F,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7F,0x01,0x01},{0x3F,0x40,0x40,0x40,0x3F},{0x1F,0x20,0x40,0x20,0x1F},
    {0x7F,0x20,0x18,0x20,0x7F},{0x63,0x14,0x08,0x14,0x63},{0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},
    {0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x60,0x60,0x00},
    {0x00,0x36,0x36,0x00,0x00}
};

static void draw_text(unsigned char *img, int W, int x, int y, const char *s) {
    while (*s) {
        int idx = 36;
        if (*s >= '0' && *s <= '9') idx = *s - '0';
        else if (*s >= 'A' && *s <= 'Z') idx = 10 + (*s - 'A');
        else if (*s >= 'a' && *s <= 'z') idx = 10 + (*s - 'a');
        else if (*s == '.') idx = 37;
        else if (*s == ':') idx = 38;

        for (int col = 0; col < 5; col++) {
            unsigned char b = FONT[idx][col];
            for (int row = 0; row < 7; row++) {
                if (b & (1 << row)) {
                    for (int dy = 0; dy < 2; dy++) {
                        for (int dx = 0; dx < 2; dx++) {
                            int px = x + col * 2 + dx, py = y + row * 2 + dy;
                            int off = (py * W + px) * 3;
                            img[off] = 0; img[off+1] = 0; img[off+2] = 0;
                        }
                    }
                }
            }
        }
        x += 14;
        s++;
    }
}

static void make_receipt_png(const Account *acc, const char *action, double amt) {
    mkdir("Receipt", 0755);
    int W = 360, H = 300;
    size_t row_len = 1 + W * 3;

    unsigned char *img = malloc(W * H * 3);
    memset(img, 255, W * H * 3);

    char buf[64];
    draw_text(img, W, 80, 25, "ATM RECEIPT");
    draw_text(img, W, 30, 65, "BANK: GLOBAL DIGITAL");
    snprintf(buf, sizeof(buf), "CARD: ****%s", acc->last4);
    draw_text(img, W, 30, 105, buf);
    snprintf(buf, sizeof(buf), "ACTION: %s", action);
    draw_text(img, W, 30, 145, buf);
    snprintf(buf, sizeof(buf), "AMOUNT: RS. %.2f", amt);
    draw_text(img, W, 30, 185, buf);
    snprintf(buf, sizeof(buf), "BALANCE: RS. %.2f", acc->balance);
    draw_text(img, W, 30, 225, buf);
    draw_text(img, W, 60, 265, "THANK YOU");

    unsigned char *raw = malloc(row_len * H);
    for (int y = 0; y < H; y++) {
        raw[y * row_len] = 0;
        memcpy(&raw[y * row_len + 1], &img[y * W * 3], W * 3);
    }
    free(img);

    char path[64];
    snprintf(path, sizeof(path), "Receipt/receipt_%ld.png", (long)time(NULL));
    FILE *fp = fopen(path, "wb");
    if (!fp) { free(raw); return; }

    const unsigned char sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    fwrite(sig, 1, 8, fp);

    unsigned char ihdr[13] = {
        (W >> 24) & 0xFF, (W >> 16) & 0xFF, (W >> 8) & 0xFF, W & 0xFF,
        (H >> 24) & 0xFF, (H >> 16) & 0xFF, (H >> 8) & 0xFF, H & 0xFF,
        8, 2, 0, 0, 0
    };
    write_chunk(fp, "IHDR", ihdr, 13);

    size_t raw_size = row_len * H;
    unsigned char *idat = malloc(raw_size + 128);
    size_t idat_len = 0;

    idat[idat_len++] = 0x78; idat[idat_len++] = 0x01;

    size_t src = 0;
    while (src < raw_size) {
        size_t blk = raw_size - src;
        if (blk > 65535) blk = 65535;
        int last = (src + blk == raw_size);
        idat[idat_len++] = last ? 0x01 : 0x00;
        idat[idat_len++] = blk & 0xFF;
        idat[idat_len++] = (blk >> 8) & 0xFF;
        idat[idat_len++] = (~blk) & 0xFF;
        idat[idat_len++] = ((~blk) >> 8) & 0xFF;
        memcpy(idat + idat_len, raw + src, blk);
        idat_len += blk;
        src += blk;
    }

    unsigned long s1 = 1, s2 = 0;
    for (size_t i = 0; i < raw_size; i++) {
        s1 = (s1 + raw[i]) % 65521;
        s2 = (s2 + s1) % 65521;
    }
    unsigned long adler = (s2 << 16) | s1;
    idat[idat_len++] = (adler >> 24) & 0xFF;
    idat[idat_len++] = (adler >> 16) & 0xFF;
    idat[idat_len++] = (adler >> 8) & 0xFF;
    idat[idat_len++] = adler & 0xFF;

    write_chunk(fp, "IDAT", idat, idat_len);
    write_chunk(fp, "IEND", NULL, 0);

    fclose(fp);
    free(raw);
    free(idat);

    show_message("RECEIPT SAVED", "PNG receipt saved in Receipt/ folder");
}

int main(void) {
    const char *JSON_FILE = "data/accounts.json";

    load_json(JSON_FILE);
    sort_accounts();
    enable_raw_mode();
    write_log(NULL, "BOOT", "ATM Simulator started");

    while (1) {
        char last4[5];
        get_4digits("ENTER LAST 4 DIGITS OF CARD", last4, 0);
        if (strlen(last4) != 4) break;

        int idx = binary_search_card(last4);
        if (idx == -1) {
            show_message("NOT FOUND", "Card not found in database.");
            write_log(last4, "SEARCH_FAIL", "Card not found");
            continue;
        }

        Account *acc = &accounts[idx];
        if (acc->is_blocked) {
            show_message("CARD BLOCKED", "Card is blocked. Contact your branch.");
            write_log(last4, "BLOCKED", "Blocked card access attempt");
            continue;
        }

        int ok = 0;
        for (int a = 1; a <= 3; a++) {
            char pin[5];
            get_4digits("ENTER 4-DIGIT PIN", pin, 1);
            if (strlen(pin) != 4) break;

            if (strcmp(pin, acc->pin) == 0) {
                ok = 1;
                acc->attempts = 0;
                save_json(JSON_FILE);
                break;
            } else {
                acc->attempts++;
                if (acc->attempts >= 3) {
                    acc->is_blocked = 1;
                    save_json(JSON_FILE);
                    show_message("CARD BLOCKED", "3 wrong PINs. Card locked!");
                    write_log(last4, "LOCKED", "Card locked (3 failed PINs)");
                    break;
                }
                char m[64];
                snprintf(m, sizeof(m), "Wrong PIN! Attempts left: %d", 3 - acc->attempts);
                show_message("INCORRECT PIN", m);
                write_log(last4, "PIN_FAIL", "Wrong PIN");
            }
        }

        if (!ok) continue;
        write_log(last4, "LOGIN", "Login successful");

        int in_session = 1;
        while (in_session) {
            char sub[64];
            snprintf(sub, sizeof(sub), "User: %s | Bal: ₹%.2f", acc->name, acc->balance);

            const char *menu[] = {
                "Check Balance",
                "Withdraw Cash",
                "Deposit Cash",
                "Eject Card & Exit"
            };

            int sel = show_menu("MAIN MENU", sub, menu, 4);

            if (sel == 0) {
                char b[64];
                snprintf(b, sizeof(b), "Available Balance: ₹%.2f", acc->balance);
                show_message("BALANCE", b);
                write_log(last4, "BALANCE", "Balance checked");
            } else if (sel == 1) {
                double amount = 0.0;
                if (get_amount("ENTER WITHDRAWAL AMOUNT", &amount)) {
                    if (amount <= acc->balance) {
                        acc->balance -= amount;
                        save_json(JSON_FILE);

                        char m[64];
                        snprintf(m, sizeof(m), "₹%.2f cash dispensed.", amount);
                        show_message("DISPENSED", m);
                        write_log(last4, "WITHDRAW", m);

                        const char *r_opts[] = { "Print PNG Receipt", "Skip Receipt" };
                        if (show_menu("RECEIPT", "Do you want a receipt?", r_opts, 2) == 0) {
                            make_receipt_png(acc, "WITHDRAWAL", amount);
                        }
                    } else {
                        show_message("DECLINED", "Insufficient balance for this amount.");
                        write_log(last4, "WITHDRAW_FAIL", "Insufficient funds");
                    }
                }
            } else if (sel == 2) {
                double amount = 0.0;
                if (get_amount("ENTER DEPOSIT AMOUNT", &amount)) {
                    acc->balance += amount;
                    save_json(JSON_FILE);

                    char m[64];
                    snprintf(m, sizeof(m), "₹%.2f deposited successfully.", amount);
                    show_message("DEPOSITED", m);
                    write_log(last4, "DEPOSIT", m);

                    const char *r_opts[] = { "Print PNG Receipt", "Skip Receipt" };
                    if (show_menu("RECEIPT", "Do you want a receipt?", r_opts, 2) == 0) {
                        make_receipt_png(acc, "DEPOSIT", amount);
                    }
                }
            } else {
                in_session = 0;
                show_message("EJECTED", "Card returned. Thank you!");
                write_log(last4, "LOGOUT", "Card ejected");
            }
        }
    }

    printf("\nATM Terminated.\n");
    return 0;
}
