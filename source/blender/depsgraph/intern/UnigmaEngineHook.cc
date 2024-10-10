#include "UnigmaEngineHook.h"

#define SHARED_MEMORY_NAME "Local\\MySharedMemory"
#define NUM_OBJECTS 10
std::map<std::string, GameObject> gameObjectMap;
namespace blender::deg {

    void HookIntoScene(Depsgraph* graph)
    {
        //PrintObjectDataFromScene(graph);
        //AssignData(graph);
        ShareData(graph);
    }

    int AssignData(Depsgraph *graph) {

              // Cast the generic Depsgraph pointer to our internal Depsgraph class.
      Depsgraph *deg_graph = reinterpret_cast<Depsgraph *>(graph);

      // Iterate over all ID nodes in the dependency graph.
      for (IDNode *id_node : deg_graph->id_nodes)
      {
            // Access the original ID (data-block) of the node.
            ID *id_orig = id_node->id_orig;

            // Check if the ID is of type Object.
            if (id_orig->name[0] == 'O' && id_orig->name[1] == 'B')
            {
                // Cast the ID to an Object.
                Object *ob_orig = reinterpret_cast<Object *>(id_orig);

                // Access the evaluated (Copy-On-Write) object to get the updated transforms.
                ID *id_cow = id_node->id_cow;
                if (id_cow != nullptr) {
                  Object *ob_eval = reinterpret_cast<Object *>(id_cow);
                  gameObjectMap[ob_orig->id.name + 2].transformMatrix = ob_eval->object_to_world();

                }
            }
        }

      return 0;
    }

int ShareData(Depsgraph *graph)
    {
      // Calculate the size of the shared memory
      const size_t sharedMemorySize = sizeof(GameObject) * NUM_OBJECTS;

      std::cout << "Size of struct: " << sizeof(GameObject) << std::endl;
      std::cout << "Size of float4: " << sizeof(blender::float4x4) << std::endl;
      // Create a file mapping object for the shared memory
      HANDLE hMapFile = CreateFileMappingA(
          INVALID_HANDLE_VALUE,  // Use paging file
          NULL,                  // Default security
          PAGE_READWRITE,        // Read/write access
          0,                     // Maximum object size (high-order DWORD)
          sharedMemorySize,      // Maximum object size (low-order DWORD)
          SHARED_MEMORY_NAME     // Name of the mapping object
      );

      if (hMapFile == NULL) {
        std::cerr << "Could not create file mapping object: " << GetLastError() << std::endl;
        return 1;
      }

      // Map a view of the file into the address space of the calling process
      GameObject *gameObjects = static_cast<GameObject *>(
          MapViewOfFile(hMapFile,             // Handle to map object
                        FILE_MAP_ALL_ACCESS,  // Read/write permission
                        0,                    // File offset high
                        0,                    // File offset low
                        sharedMemorySize      // Number of bytes to map
                        ));

      if (gameObjects == NULL) {
        std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        return 1;
      }

      // Assume Depsgraph* graph is properly set up and accessible here
      int index = 0;
      for (IDNode *id_node : graph->id_nodes) {
        if (index >= NUM_OBJECTS) {
          break;  // Prevent writing beyond allocated shared memory
        }

        // Access the original ID (data-block) of the node
        ID *id_orig = id_node->id_orig;

        // Check if the ID is of type Object
        if (id_orig->name[0] == 'O' && id_orig->name[1] == 'B') {
          Object *ob_orig = reinterpret_cast<Object *>(id_orig);

          // Access the evaluated (Copy-On-Write) object to get the updated transforms
          ID *id_cow = id_node->id_cow;
          if (id_cow != nullptr) {
            Object *ob_eval = reinterpret_cast<Object *>(id_cow);

            // Prepare GameObject data
            gameObjects[index].id = 0;
            for (int i = 0; i < 66; i++) {
              gameObjects[index].name[i] = ob_eval->id.name[i];
            }
            gameObjects[index].transformMatrix = ob_eval->object_to_world();
            // Optional: Print to verify
            std::cout << "Name: " << ob_eval->id.name << std::endl;
            std::cout << "Object Name: " << gameObjects[index].name+2 << std::endl;
            std::cout << "Wrote Object[" << index << "] id: " << gameObjects[index].id << std::endl;
            for (int row = 0; row < 4; ++row) {
              std::cout << "[ ";
              for (int col = 0; col < 4; ++col) {
                std::cout << gameObjects[index].transformMatrix[row][col] << " ";
              }
              std::cout << "]\n";
            }

            index++;
          }
        }
      }

      // Clean up
      UnmapViewOfFile(gameObjects);
      CloseHandle(hMapFile);

      return 0;
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
