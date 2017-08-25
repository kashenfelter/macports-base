/*
 * snapshot.c
 * vim:tw=80:expandtab
 *
 * Copyright (c) 2017 The MacPorts Project
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>
#include <tcl.h>
#include <sqlite3.h>

#include <cregistry/util.h>

#include "snapshot.h"
#include "snapshotobj.h"
#include "registry.h"
#include "util.h"

/**
 * Converts a command name into a `reg_snapshot`.
 *
 * @param [in] interp  Tcl interpreter to check within
 * @param [in] name    name of snapshot to get
 * @param [out] errPtr description of error if the snapshot can't be found
 * @return             an snapshot, or NULL if one couldn't be found
 * @see get_object
 */
static reg_snapshot* get_snapshot(Tcl_Interp* interp, char* name, reg_error* errPtr) {
    return (reg_snapshot*)get_object(interp, name, "snapshot", snapshot_obj_cmd, errPtr);
}

/**
 * Removes the snapshot from the Tcl interpreter. Doesn't actually delete it since
 * that's the registry's job. This is written to be used as the
 * `Tcl_CmdDeleteProc` for a snapshot object command.
 *
 * @param [in] clientData address of a reg_snapshot to remove
 */
void delete_snapshot(ClientData clientData) {
    reg_snapshot* snapshot = (reg_snapshot*)clientData;
    free(snapshot->proc);
    snapshot->proc = NULL;
}

/*
 * registry::snaphot create note
 * note is required
 */
static int snapshot_create(Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]) {

    reg_registry* reg = registry_for(interp, reg_attached);
    if (objc > 3) {
        Tcl_WrongNumArgs(interp, 2, objv, "create_snapshot ?note?");
        return TCL_ERROR;
    } else if (reg == NULL) {
        return TCL_ERROR;
    } else {
        char* note = Tcl_GetString(objv[2]);
        reg_error error;
        reg_snapshot* new_snaphot = reg_snapshot_create(reg, note, &error);
        if (new_snaphot != NULL) {
            Tcl_Obj* result;
            if (snapshot_to_obj(interp, &result, new_snaphot, NULL, &error)) {
                Tcl_SetObjResult(interp, result);
                return TCL_OK;
            }
        }
        return registry_failed(interp, &error);
    }
}

// static int get_snapshot_by_id(Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]) {

//     printf("getting snapshot\n");

//     reg_registry* reg = registry_for(interp, reg_attached);
//     if (objc > 3) {
//         Tcl_WrongNumArgs(interp, 2, objv, "get_by_id ?snapshot_id?");
//         return TCL_ERROR;
//     } else if (reg == NULL) {
//         return TCL_ERROR;
//     } else {
//         sqlite_int64 id = atoll(Tcl_GetString(objv[2]));
//         reg_error error;
//         reg_snapshot* snapshot = NULL;
//         int port_count = reg_snapshot_get(snapshot, id, snapshot, &error);
//         if (snapshot != NULL && port_count >= 0) {
//             Tcl_Obj* resultObj;
//             if (snapshot_to_obj(interp, &resultObj, snapshot, NULL, &error)) {
//                 Tcl_SetObjResult(interp, resultObj);
//                 return TCL_OK;
//             }
//         }
//         return registry_failed(interp, &error);
//     }
// }

typedef struct {
    char* name;
    int (*function)(Tcl_Interp* interp, int objc, Tcl_Obj* CONST objv[]);
} snapshot_cmd_type;

static snapshot_cmd_type snapshot_cmds[] = {
    /* Global commands */
    { "create", snapshot_create},
    // { "get_by_id", get_snapshot_by_id},
    { NULL, NULL }
};

/*
 * registry::snapshot cmd ?arg ...?
 *
 * Commands manipulating snapshots in the registry.
 */
int snapshot_cmd(ClientData clientData UNUSED, Tcl_Interp* interp, int objc,
        Tcl_Obj* CONST objv[]) {
    int cmd_index;
    if (objc < 2) {
        Tcl_WrongNumArgs(interp, 1, objv, "cmd ?arg ...?");
        return TCL_ERROR;
    }
    if (Tcl_GetIndexFromObjStruct(interp, objv[1], snapshot_cmds,
                sizeof(snapshot_cmd_type), "cmd", 0, &cmd_index) == TCL_OK) {
        snapshot_cmd_type* cmd = &snapshot_cmds[cmd_index];
        return cmd->function(interp, objc, objv);
    }
    return TCL_ERROR;
}
