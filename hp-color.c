#include <linux/module.h> /* Needed by all modules */
#include <linux/printk.h> /* Needed for pr_info() */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/input/sparse-keymap.h>
#include <linux/platform_device.h>
#include <linux/acpi.h>
#include <linux/rfkill.h>
#include <linux/string.h>

#define HPWMI_BIOS_GUID "5FB7F034-2C63-45e9-BE91-3D44E2C707E4"

struct bios_args {
    u32 signature;
    u32 command;
    u32 commandtype;
    u32 datasize;
    u8 data[128];
};

enum hp_wmi_command {
    HPWMI_READ = 0x01,
    HPWMI_WRITE = 0x02,
    HPWMI_ODM = 0x03,
    HPWMI_COLOR = 131081,
};

enum hp_light {
    ON = 0,
    BREATHING = 1,
    BLINKING = 3,
    OFF = 255,
};

enum hp_animation_duration {
    SHORT = 2,
    MEDIUM = 5,
    LONG = 10,
};

/* map output size to the corresponding WMI method id */
static inline int encode_outsize_for_pvsz(int outsize)
{
  if (outsize > 4096)
    return -EINVAL;
  if (outsize > 1024)
    return 5;
  if (outsize > 128)
    return 4;
  if (outsize > 4)
    return 3;
  if (outsize > 0)
    return 2;
  return 1;
}

enum hp_return_value {
  HPWMI_RET_WRONG_SIGNATURE	= 0x02,
  HPWMI_RET_UNKNOWN_COMMAND	= 0x03,
  HPWMI_RET_UNKNOWN_CMDTYPE	= 0x04,
  HPWMI_RET_INPUT_SIZE_NULL	= 0x05,
  HPWMI_RET_INPUT_DATA_NULL = 0x06,
  HPWMI_RET_INPUT_DATA_INVALID = 0x07,
  HPWMI_RET_RETURN_SIZE_NULL	= 0x08,
  HPWMI_RET_RETURN_SIZE_INVALID = 0x09,
};

struct bios_return {
  u32 sigpass;
  u32 return_code;
};

/*
 * hp_wmi_perform_query
 *
 * query:	The commandtype (enum hp_wmi_commandtype)
 * write:	The command (enum hp_wmi_command)
 * buffer:	Buffer used as input and/or output
 * insize:	Size of input buffer
 * outsize:	Size of output buffer
 *
 * returns zero on success
 *         an HP WMI query specific error code (which is positive)
 *         -EINVAL if the query was not successful at all
 *         -EINVAL if the output buffer size exceeds buffersize
 *
 * Note: The buffersize must at least be the maximum of the input and output
 *       size. E.g. Battery info query is defined to have 1 byte input
 *       and 128 byte output. The caller would do:
 *       buffer = kzalloc(128, GFP_KERNEL);
 *       ret = hp_wmi_perform_query(HPWMI_BATTERY_QUERY, HPWMI_READ, buffer, 1, 128)
 */
static int hp_wmi_perform_query(int query, enum hp_wmi_command command,
                                void *buffer, int insize, int outsize) {
    int mid;
    struct bios_return *bios_return;
    int actual_outsize;
    union acpi_object *obj;
    struct bios_args args = {
            .signature = 0x55434553,
            .command = command,
            .commandtype = query,
            .datasize = insize,
            .data = {0},
    };
    struct acpi_buffer input = {sizeof(struct bios_args), &args};
    struct acpi_buffer output = {ACPI_ALLOCATE_BUFFER, NULL};
    int ret = 0;

    mid = encode_outsize_for_pvsz(outsize);
    if (WARN_ON(mid < 0))
        return mid;

    if (WARN_ON(insize > sizeof(args.data)))
        return -EINVAL;
    memcpy(&args.data[0], buffer, insize);

    wmi_evaluate_method(HPWMI_BIOS_GUID, 0, mid, &input, &output);

    obj = output.pointer;

    if (!obj)
        return -EINVAL;

    if (obj->type != ACPI_TYPE_BUFFER) {
        ret = -EINVAL;
        goto out_free;
    }

    bios_return = (struct bios_return *) obj->buffer.pointer;
    ret = bios_return->return_code;

    if (ret) {
        if (ret != HPWMI_RET_UNKNOWN_CMDTYPE)
            pr_warn("query 0x%x returned error 0x%x\n", query, ret);
        goto out_free;
    }

    /* Ignore output data of zero size */
    if (!outsize)
        goto out_free;

    actual_outsize = min(outsize, (int) (obj->buffer.length - sizeof(*bios_return)));
    memcpy(buffer, obj->buffer.pointer + sizeof(*bios_return), actual_outsize);
    memset(buffer + actual_outsize, 0, outsize - actual_outsize);

    out_free:
    kfree(obj);
    return ret;
}


static int __init start_module(void) {
    u8 state[3] = { 0, 0, 0 };
    int ret;
    int bios_capable = wmi_has_guid(HPWMI_BIOS_GUID);

    pr_info("HP-Color module loaded.\n");
    if (!bios_capable) {
        pr_err("BIOS not capable!\n");
        return 1;
    }

    state[1] = OFF;  // Change animation
    state[2] = LONG; // Change duration of animation

    ret = hp_wmi_perform_query(7, HPWMI_COLOR, &state, sizeof(state), sizeof(state));
    //pr_info("Query return: %d\n", ret);

    return 0;
}

static void __exit exit_module(void) {
    pr_info("HP-Color module unloaded.\n");
}


module_init(start_module);
module_exit(exit_module);

MODULE_LICENSE("GPL");
