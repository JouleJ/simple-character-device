#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#define OUTPUT_QUEUE_SIZE 1024
#define INPUT_BUF_SIZE 1024

MODULE_LICENSE("GPL");

static int major_num;

static ssize_t device_read(struct file* flip, char* buffer, size_t len, loff_t* offset);
static ssize_t device_write(struct file* flip, const char* buffer, size_t len, loff_t* offset);
static int device_open(struct inode* inode, struct file* file);
static int device_release(struct inode* inode, struct file* file);

static struct file_operations file_ops = {
    .open = device_open,
    .release = device_release,
    .read = device_read,
    .write = device_write
};

static char out_queue[OUTPUT_QUEUE_SIZE];
static int out_queue_begin = 0;
static int out_queue_end = 0;

static char input_buf[INPUT_BUF_SIZE + 1];

static void output_char(char chr) {
    out_queue[out_queue_end++] = chr;
    out_queue_end = (out_queue_end == OUTPUT_QUEUE_SIZE ? 0 : out_queue_end);
}

static void output_str(const char* str) {
    while (*str) {
        output_char(*(str++));
    }
}

struct person {
    char* first_name;
    char* last_name;
    char* age;
    char* phone_number;
    char* email;
    struct list_head list;
};

static int isspace(int c) {
    return (c >= 9 && c <= 13) || (c == 32);
}

static char* parse_field(char** str) {
    char *ptr, *s, *t;
    int count = 0;
    while (isspace(**str)) {
        ++(*str);
    }
    ptr = *str;
    while (*ptr && !isspace(*ptr)) {
        ++count;
        ++ptr;
    }
    if (!count) {
        return NULL;
    }
    s = kmalloc(sizeof(*s) * (count + 1), GFP_KERNEL);
    s[count] = 0;
    t = s;
    while (**str && !isspace(**str)) {
        *t = **str;
        ++t;
        ++(*str);
    }
    return s;
}

static void free_person(struct person* p) {
    if (p) {
        kfree(p->first_name);
        kfree(p->last_name);
        kfree(p->age);
        kfree(p->phone_number);
        kfree(p->email);
        kfree(p);
    }
}

static struct person* parse_person(char* str) {
    struct person* p = kmalloc(sizeof(*p), GFP_KERNEL);
    p->first_name = parse_field(&str);   
    p->last_name = parse_field(&str);   
    p->age = parse_field(&str);   
    p->phone_number = parse_field(&str);   
    p->email = parse_field(&str);
    while (*str && isspace(*str)) {
        ++str;
    }
    if (*str || !p->first_name || !p->last_name || !p->age || !p->phone_number || !p->email) {
        free_person(p);
        return NULL;
    }
    return p;
}

LIST_HEAD(people_list);

static void insert_person(struct person* p) {
    INIT_LIST_HEAD(&p->list);
    list_add(&p->list, &people_list);
}

static void output_person(struct person* p) {
    output_str("first name: ");
    output_str(p->first_name);
    output_str("\nlast name: ");
    output_str(p->last_name);
    output_str("\nage: ");
    output_str(p->age);
    output_str("\nphone number: ");
    output_str(p->phone_number);
    output_str("\nemail: ");
    output_str(p->email);
    output_char('\n');
}

struct person* find_by_lastname(const char* last_name) {
    struct person* q;
    struct list_head* node = people_list.next;
    while (node != &people_list) {
        q = container_of(node, struct person, list);
        if (!strcmp(q->last_name, last_name)) {
            return q;
        }
        node = node->next;
    }
    return NULL;
}

#define CMD_GET     1
#define CMD_INSERT  2
#define CMD_REMOVE  3

static void handle_input(int input_size) {
    char *data, *cmd, *query;
    int cmd_num = -1;
    struct person *p;
    input_buf[input_size] = 0;
    data = &input_buf[0];
    cmd = parse_field(&data);
    if (cmd) {
        if (!strcmp(cmd, "get")) {
            cmd_num = CMD_GET;
        } else if (!strcmp(cmd, "insert")) {
            cmd_num = CMD_INSERT;
        } else if (!strcmp(cmd, "remove")) {
            cmd_num = CMD_REMOVE;
        }
        kfree(cmd);
        if (cmd_num == CMD_GET) {
            query = parse_field(&data);
            if (query) {
                p = find_by_lastname(query);
                if (!p) {
                    output_str("Nothing found\n");
                } else {
                    output_person(p);
                }
                kfree(query);
            }
        } else if (cmd_num == CMD_INSERT) {
            p = parse_person(data);
            if (p) {
                insert_person(p);
            }
        } else if (cmd_num == CMD_REMOVE) {
            query = parse_field(&data);
            if (query) {
                p = find_by_lastname(query);
                if (p) {
                    list_del(&p->list);
                }
                kfree(query);
            }
        }
    }
}

static int __init phone_book_init(void) {
    major_num = register_chrdev(0, "phone_book", &file_ops);
    if (major_num < 0) {
        printk(KERN_ALERT "(phone_book) Failed to register chrdev: %d\n", major_num);
        return major_num;
    } else {
        printk(KERN_INFO "(phone_book) Registered chrdev: %d\n", major_num);
        return 0;
    }
}

static void __exit phone_book_exit(void) {
    struct person* p;
    while (people_list.next != &people_list) {
        struct list_head* node = people_list.next;
        list_del(node);
        p = container_of(node, struct person, list);
    }
    unregister_chrdev(major_num, "phone_book");
    printk(KERN_INFO "(phone_book) Exited");
}

static ssize_t device_read(struct file* flip, char* buffer, size_t len, loff_t* offset) {
    ssize_t bytes_read = 0;
    while (out_queue_begin != out_queue_end && len) {
        put_user(out_queue[out_queue_begin++], buffer++);
        ++bytes_read;
        --len;
        out_queue_begin = (out_queue_begin == OUTPUT_QUEUE_SIZE ? 0 : out_queue_begin);
    }
    return bytes_read;
}

static ssize_t device_write(struct file* flip, const char* buffer, size_t len, loff_t* offset) {
    if (len > INPUT_BUF_SIZE) {
        printk(KERN_ALERT "(phone_book) Input to large: %zu bytes\n", len);
        return -ENOBUFS;
    }
    if (copy_from_user(input_buf, buffer, len)) {
        return -EFAULT;
    }
    handle_input(len);
    return len;
}

static int device_open(struct inode* inode, struct file* file) {
    return 0;
}

static int device_release(struct inode* inode, struct file* file) {
    return 0;
}

module_init(phone_book_init);
module_exit(phone_book_exit);
