#include "UnigmaEngineHook.h"
#include <cstdio>

namespace blender::deg {

    void HookIntoScene(Depsgraph* graph)
    {
        PrintObjectDataFromScene(graph);
    }

    void PrintObjectDataFromScene(Depsgraph* graph)
    {
        printf("Unigma Scene!!!");

        // Cast the generic Depsgraph pointer to our internal Depsgraph class.
        Depsgraph* deg_graph = reinterpret_cast<Depsgraph*>(graph);

        // Iterate over all ID nodes in the dependency graph.
        for (IDNode* id_node : deg_graph->id_nodes) {
            // Access the original ID (data-block) of the node.
            ID* id_orig = id_node->id_orig;

            // Check if the ID is of type Object.
            if (id_orig->name[0] == 'O' && id_orig->name[1] == 'B') { // 'OB' prefix for objects
                // Cast the ID to an Object.
                Object* ob_orig = reinterpret_cast<Object*>(id_orig);

                // Print the object's name.
                printf("Object name: %s\n", ob_orig->id.name + 2); // Skip the 'OB' prefix.

                // Access the evaluated (Copy-On-Write) object to get the updated transforms.
                ID* id_cow = id_node->id_cow;
                if (id_cow != nullptr) {
                    Object* ob_eval = reinterpret_cast<Object*>(id_cow);

                    // Extract the location from the object's transformation matrix.
                    float loc[3];
                    loc[0] = ob_eval->object_to_world()[3][0];
                    loc[1] = ob_eval->object_to_world()[3][1];
                    loc[2] = ob_eval->object_to_world()[3][2];

                    // Print the location.
                    printf(" - Location: (%f, %f, %f)\n", loc[0], loc[1], loc[2]);

                    // Print the rotation matrix.
                    printf(" - Rotation matrix:\n");
                    for (int i = 0; i < 3; i++) {
                        printf("   [%f %f %f]\n",
                            ob_eval->object_to_world()[i][0],
                            ob_eval->object_to_world()[i][1],
                            ob_eval->object_to_world()[i][2]);
                    }
                }
                else {
                    printf(" - Evaluated object is null.\n");
                }
            }
        }
    }
}