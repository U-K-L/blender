#pragma once

// Include necessary Blender headers
#include "intern/depsgraph.hh"
#include "intern/depsgraph_relation.hh"
#include "intern/depsgraph_tag.hh"
#include "intern/eval/deg_eval_copy_on_write.h"
#include "intern/eval/deg_eval_flush.h"
#include "intern/eval/deg_eval_stats.h"
#include "intern/eval/deg_eval_visibility.h"
#include "intern/node/deg_node.hh"
#include "intern/node/deg_node_component.hh"
#include "intern/node/deg_node_id.hh"
#include "intern/node/deg_node_operation.hh"
#include "intern/node/deg_node_time.hh"

#include "intern/eval/deg_eval.h"

#include "BLI_compiler_attrs.h"
#include "BLI_function_ref.hh"
#include "BLI_gsqueue.h"
#include "BLI_task.h"
#include "BLI_time.h"
#include "BLI_utildefines.h"

#include "BKE_global.hh"

#include "DNA_node_types.h"
#include "DNA_object_types.h"
#include "DNA_scene_types.h"

#include "DEG_depsgraph.hh"
#include "DEG_depsgraph_query.hh"

#include <stdio.h>
#include "BKE_object.hh"

#include <cstdio>
#include <windows.h>
#include <iostream>
#include <stdint.h>

#define MAX_VERTICES 10000
#define MAX_INDICES 10000

struct GameObject {
  char name[66];
  char _pad[14];
  blender::float4x4 transformMatrix;
  int id;
  int vertexCount;
  int indexCount;
  blender::float3 vertices[MAX_VERTICES];
  uint32_t indices[MAX_INDICES];
};


namespace blender::deg {
    extern "C" {

        void PrintObjectDataFromScene(Depsgraph* graph);
        void HookIntoScene(Depsgraph* graph);
        int ShareData(Depsgraph *graph);
        int AssignData(Depsgraph *graph);

    } // extern "C"
}
