#pragma once
#include <Engine/BlockingQueue.h>
#include <Engine/math.h>
#include <thread>

class ChunkGroup;
class ChunkManager;
class World;

class TerrainGenerator {
public:
    TerrainGenerator(World& world, ChunkManager& chunkManager);

    void run();
    void stop();

    bool enqueue(glm::ivec2 coord);

private:
    static const size_t queueSize = 16;
    VoxelEngine::BlockingQueue<glm::ivec2> m_queue;
    World* m_world;
    ChunkManager* m_chunkManager;
    bool m_running = false;
    std::thread m_thread;

    void loop();
    void generate(glm::ivec2 coord);
};