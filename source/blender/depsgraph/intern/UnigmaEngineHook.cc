#include "UnigmaEngineHook.h"
#include "BKE_mesh.h"        // Include this for mesh-related functions
#include "DNA_mesh_types.h"  // Include this for Mesh definitions

#define DATA_READY_EVENT_NAME "Local\\DataReadyEvent"
#define READ_COMPLETE_EVENT_NAME "Local\\ReadCompleteEvent"
#define SHARED_MEMORY_NAME "Local\\MySharedMemory"
#define NUM_OBJECTS 10
std::map<std::string, RenderObject> gameObjectMap;
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

// Return types:
// 0: Success
// -1: Error (general)
// 1: No File
// 2: Data not ready.
// 3: Edit mode.
int ShareData(Depsgraph *graph)
{
      // Create events for synchronization
      HANDLE hDataReadyEvent = CreateEventA(
        NULL, // Default security attributes
        FALSE, // Auto-reset event
        FALSE, // Initial state is non-signaled
        DATA_READY_EVENT_NAME // Name of the event
      );
      // Create the event with manual reset and initial state signaled
      HANDLE hReadCompleteEvent = CreateEventA(
        NULL, // Default security attributes
        TRUE, // Manual-reset event
        TRUE, // Initial state is signaled
        READ_COMPLETE_EVENT_NAME  // Name of the event
      );

    if (hDataReadyEvent == NULL || hReadCompleteEvent == NULL) {
        std::cerr << "Could not create events: " << GetLastError() << std::endl;
        return 1;
      }

      // Calculate the size of the shared memory
      const size_t sharedMemorySize = sizeof(RenderObject) * NUM_OBJECTS;

      std::cout << "Size of struct: " << sizeof(RenderObject) << std::endl;
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
        CloseHandle(hDataReadyEvent);
        CloseHandle(hReadCompleteEvent);
          return 1;
      }

      // Map a view of the file into the address space of the calling process
      RenderObject *renderObjects = static_cast<RenderObject *>(
          MapViewOfFile(hMapFile,             // Handle to map object
                        FILE_MAP_ALL_ACCESS,  // Read/write permission
                        0,                    // File offset high
                        0,                    // File offset low
                        sharedMemorySize      // Number of bytes to map
                        ));

      if (renderObjects == NULL) {
        std::cerr << "Could not map view of file: " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        CloseHandle(hDataReadyEvent);
        CloseHandle(hReadCompleteEvent);
        return 1;
      }

      //Begin to read and write
      DWORD waitResult = WaitForSingleObject(hReadCompleteEvent, 0);  // Zero timeout for non-blocking
      if (waitResult != WAIT_OBJECT_0) {
        std::cerr << "File still being read. " << GetLastError() << std::endl;
        CloseHandle(hMapFile);
        CloseHandle(hDataReadyEvent);
        CloseHandle(hReadCompleteEvent);
        return 2;
      }

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

          if ((ob_orig->mode & OB_MODE_EDIT)) {
            CloseHandle(hMapFile);
            CloseHandle(hDataReadyEvent);
            CloseHandle(hReadCompleteEvent);
            return 3;
          }
          // Access the evaluated (Copy-On-Write) object to get the updated transforms
          ID *id_cow = id_node->id_cow;



          if (id_cow != nullptr) {
            Object *ob_eval = reinterpret_cast<Object *>(id_cow);

            // Prepare GameObject data
            // Store the name.
            renderObjects[index].id = 0;
            for (int i = 0; i < 66; i++) {
              renderObjects[index].name[i] = ob_eval->id.name[i];
            }

            // Transform matrix.
            renderObjects[index].transformMatrix = ob_eval->object_to_world();

            // Get Mesh data.
            if (ob_orig->type == OB_MESH) {
              Object *ob_eval = reinterpret_cast<Object *>(id_cow);
              Mesh *mesh = BKE_object_get_evaluated_mesh(ob_eval);

                if (mesh != nullptr) {
                // Copy object name
                strncpy(renderObjects[index].name,
                        ob_eval->id.name + 2,
                        sizeof(renderObjects[index].name) - 1);

                // Access vertex positions
                blender::Span<blender::float3> meshVerts = mesh->vert_positions();

                // Copy vertices and print them
                renderObjects[index].vertexCount = mesh->verts_num;
                for (int i = 0; i < mesh->verts_num; ++i) {
                  blender::float3 vertex = meshVerts[i];
                  renderObjects[index].vertices[i] = vertex;

                  /*
                  // Print vertex position
                  std::cout << "Vertex " << i << ": " << vertex.x << ", " << vertex.y << ", "
                            << vertex.z << std::endl;
                            */
                }

                // Access edges
                blender::Span<blender::int2> meshEdges = mesh->edges();

                // Copy indices and print them
                int face_index = 0;
                for (int i = 0; i < meshEdges.size(); ++i) {
                  blender::int2 edge = meshEdges[i];
                  int vertex_index_0 = edge[0];  // or edge.x
                  int vertex_index_1 = edge[1];  // or edge.y

                  // Store the edge's vertex indices
                  renderObjects[index].indices[face_index++] = vertex_index_0;
                  renderObjects[index].indices[face_index++] = vertex_index_1;

                  /*
                  // Print edge indices
                  std::cout << "Edge " << i << ": " << vertex_index_0 << " - " << vertex_index_1
                            << std::endl;
                            */
                }
              }

            }

            /*
            //Print to verify
            std::cout << "Name: " << ob_eval->id.name << std::endl;
            std::cout << "Object Name: " << gameObjects[index].name+2 << std::endl;
            std::cout << "Wrote Object[" << index << "] id: " << gameObjects[index].id << std::endl;

            //Print matrix
            for (int row = 0; row < 4; ++row) {
              std::cout << "[ ";
              for (int col = 0; col < 4; ++col) {
                std::cout << gameObjects[index].transformMatrix[row][col] << " ";
              }
              std::cout << "]\n";
            }
            */
            index++;
          }
        }
      }

      SetEvent(hDataReadyEvent);
      ResetEvent(hReadCompleteEvent);

      // Clean up
      //UnmapViewOfFile(renderObjects);
      //CloseHandle(hMapFile);
      //CloseHandle(hDataReadyEvent);
      //CloseHandle(hReadCompleteEvent);


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
