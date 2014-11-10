/*
 * Copyright (c) Michael Tharp <gxti@partiallystapled.com>
 *
 * This file is distributed under the terms of the MIT License.
 * See the LICENSE file at the top of this tree, or if it is missing a copy can
 * be found at http://opensource.org/licenses/MIT
 */

#include "common.h"
#include "status.h"
#include "vtimer.h"
#include "gps/parser.h"
#include "lwip/snmp.h"
#include "lwip/snmp_asn1.h"
#include "lwip/snmp_structs.h"

#if LWIP_SNMP

static void
loopstats_get_object_def(uint8_t ident_len, int32_t *ident, struct obj_def *od) {
    ident_len++;
    ident--;
    if (ident_len != 2) {
        od->instance = MIB_OBJECT_NONE;
        return;
    }
    od->id_inst_len = ident_len;
    od->id_inst_ptr = ident;
    switch (ident[0]) {
        case 1: /* timeOffset */
        case 2: /* frequencyOffset */
        case 3: /* timeJitter */
        case 4: /* frequencyJitter */
        case 5: /* loopTimeConstant */
        case 6: /* pllState */
            od->instance = MIB_OBJECT_TAB;
            od->access = MIB_OBJECT_READ_ONLY;
            od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
            od->v_len = sizeof(int32_t);
            return;
        default:
            od->instance = MIB_OBJECT_NONE;
            return;
    }
}


static void
loopstats_get_value(struct obj_def *od, uint16_t len, void *value) {
    uint8_t id = od->id_inst_ptr[0];
    if (id >= 1 && id <= LOOPSTATS_VALUES + 1) {
        int32_t *sint_ptr = (int32_t*)value;
        *sint_ptr = loopstats_values[id - 1];
    }
}



static void
gps_get_object_def(uint8_t ident_len, int32_t *ident, struct obj_def *od) {
    ident_len++;
    ident--;
    if (ident_len != 2) {
        od->instance = MIB_OBJECT_NONE;
        return;
    }
    od->id_inst_len = ident_len;
    od->id_inst_ptr = ident;
    switch (ident[0]) {
        case 1: /* numSvInFix */
            od->instance = MIB_OBJECT_TAB;
            od->access = MIB_OBJECT_READ_ONLY;
            od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
            od->v_len = sizeof(int32_t);
            return;
        default:
            od->instance = MIB_OBJECT_NONE;
            return;
    }
}


static void
gps_get_value(struct obj_def *od, uint16_t len, void *value) {
    uint8_t id = od->id_inst_ptr[0];
    int32_t *sint_ptr = (int32_t*)value;
    switch (id) {
        case 1: /* numSvInFix */
            *sint_ptr = gps_fix_svs;
            break;
    }
}


static void
serverstate_get_object_def(uint8_t ident_len, int32_t *ident, struct obj_def *od) {
    ident_len++;
    ident--;
    if (ident_len == 2) {
        od->id_inst_len = ident_len;
        od->id_inst_ptr = ident;
        od->instance = MIB_OBJECT_SCALAR;
        od->access = MIB_OBJECT_READ_ONLY;
        od->asn_type = (SNMP_ASN1_UNIV | SNMP_ASN1_PRIMIT | SNMP_ASN1_INTEG);
        od->v_len = sizeof(int32_t);
    } else {
        od->instance = MIB_OBJECT_NONE;
    }
}


static void
serverstate_get_value(struct obj_def *od, uint16_t len, void *value) {
    int32_t *sint_ptr = (int32_t*)value;
    *sint_ptr = (int32_t)status_flags;
}


/* loopStats .1.3.6.1.4.1.x.1.2 */
static const mib_scalar_node mib_loopstats_scalar = {
    &loopstats_get_object_def,
    &loopstats_get_value,
    &noleafs_set_test,
    &noleafs_set_value,
    MIB_NODE_SC,
    0
};
static const s32_t mib_loopstats_ids[6] = { 1, 2, 3, 4, 5, 6 };
static struct mib_node* const mib_loopstats_nodes[6] = {
    (struct mib_node*)&mib_loopstats_scalar,
    (struct mib_node*)&mib_loopstats_scalar,
    (struct mib_node*)&mib_loopstats_scalar,
    (struct mib_node*)&mib_loopstats_scalar,
    (struct mib_node*)&mib_loopstats_scalar,
    (struct mib_node*)&mib_loopstats_scalar
    };
static const struct mib_array_node mib_loopstats = {
    &noleafs_get_object_def,
    &noleafs_get_value,
    &noleafs_set_test,
    &noleafs_set_value,
    MIB_NODE_AR,
    6,
    mib_loopstats_ids,
    mib_loopstats_nodes
};

/* gps .1.3.6.1.4.1.x.1.3 */
static const mib_scalar_node mib_gps_scalar = {
    &gps_get_object_def,
    &gps_get_value,
    &noleafs_set_test,
    &noleafs_set_value,
    MIB_NODE_SC,
    0
};
static const s32_t mib_gps_ids[1] = { 1 };
static struct mib_node* const mib_gps_nodes[1] = {
    (struct mib_node*)&mib_gps_scalar,
    };
static const struct mib_array_node mib_gps = {
    &noleafs_get_object_def,
    &noleafs_get_value,
    &noleafs_set_test,
    &noleafs_set_value,
    MIB_NODE_AR,
    1,
    mib_gps_ids,
    mib_gps_nodes
};

/* ntpServer .1.3.6.1.4.1.x.1 */
static const mib_scalar_node mib_ntpserver_scalar = {
    &serverstate_get_object_def,
    &serverstate_get_value,
    &noleafs_set_test,
    &noleafs_set_value,
    MIB_NODE_SC,
    0
};
static const s32_t mib_ntpserver_ids[3] = { 1, 2, 3 };
static struct mib_node* const mib_ntpserver_nodes[3] = {
    (struct mib_node*)&mib_ntpserver_scalar,
    (struct mib_node*)&mib_loopstats,
    (struct mib_node*)&mib_gps,
    };
static const struct mib_array_node mib_ntpserver = {
    &noleafs_get_object_def,
    &noleafs_get_value,
    &noleafs_set_test,
    &noleafs_set_value,
    MIB_NODE_AR,
    3,
    mib_ntpserver_ids,
    mib_ntpserver_nodes
};

/* (vendor) .1.3.6.1.4.1.x */
static const s32_t mib_vendor_ids[1] = { 1 };
static struct mib_node* const mib_vendor_nodes[1] = { (struct mib_node*)&mib_ntpserver };
static const struct mib_array_node mib_vendor = {
    &noleafs_get_object_def,
    &noleafs_get_value,
    &noleafs_set_test,
    &noleafs_set_value,
    MIB_NODE_AR,
    1,
    mib_vendor_ids,
    mib_vendor_nodes
};

/* enterprises .1.3.6.1.4.1 */
static const s32_t mib_enterprises_ids[1] = { 29174 }; /* XXX FIXME */
static struct mib_node* const mib_enterprises_nodes[1] = { (struct mib_node*)&mib_vendor };
static const struct mib_array_node mib_enterprises = {
    &noleafs_get_object_def,
    &noleafs_get_value,
    &noleafs_set_test,
    &noleafs_set_value,
    MIB_NODE_AR,
    1,
    mib_enterprises_ids,
    mib_enterprises_nodes
};

/* private .1.3.6.1.4 */
static const s32_t mib_private_ids[1] = { 1 };
static struct mib_node* const mib_private_nodes[1] = { (struct mib_node*)&mib_enterprises };
const struct mib_array_node mib_private = {
    &noleafs_get_object_def,
    &noleafs_get_value,
    &noleafs_set_test,
    &noleafs_set_value,
    MIB_NODE_AR,
    1,
    mib_private_ids,
    mib_private_nodes
};

#endif
