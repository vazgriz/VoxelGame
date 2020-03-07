#include "ChunkMesher.h"
#include "ChunkMesh.h"

ChunkMesher::ChunkMesher(VoxelEngine::Engine& engine, World& world, BlockManager& blockManager, MeshManager& meshManager) : m_requestQueue(queueSize) {
    m_engine = &engine;
    m_world = &world;
    m_blockManager = &blockManager;
    m_meshManager = &meshManager;
}

void ChunkMesher::setTransferNode(VoxelEngine::TransferNode& transferNode) {
    m_transferNode = &transferNode;
}

void ChunkMesher::update(VoxelEngine::Clock& clock) {
    std::queue<MeshUpdate2>& queue = m_resultQueue.swapDequeue();
    auto view = m_world->registry().view<Chunk, ChunkMesh>();

    while (queue.size() > 0) {
        auto& update = queue.front();
        auto entity = m_world->getEntity(update.coord);
        if (entity != entt::null) {
            transferMesh(view.get<ChunkMesh>(entity), update.index);
        }
        queue.pop();
    }
}

void ChunkMesher::run() {
    m_running = true;
    m_thread = std::thread([this] { loop(); });
}

void ChunkMesher::stop() {
    m_running = false;
    m_requestQueue.cancel();
    m_thread.join();
}

bool ChunkMesher::queue(glm::ivec3 coord) {
    return m_requestQueue.tryEnqueue(coord);
}

void ChunkMesher::loop() {
    while (m_running) {
        glm::ivec3 coord;
        bool valid = m_requestQueue.dequeue(coord);
        if (!valid) return;

        update(coord);
    }
}

void ChunkMesher::update(glm::ivec3 worldChunkPos) {
    ChunkBuffer blocks;
    LightBuffer light;
    ChunkData<Chunk*, 3> neighborChunks;
    const glm::ivec3 root = { 1, 1, 1 };
    std::queue<LightUpdate> queue;

    {
        auto lock = m_world->getLock();
        entt::entity entity = m_world->getEntity(worldChunkPos);
        if (entity == entt::null) return;

        auto view = m_world->registry().view<Chunk, ChunkMesh>();
        Chunk& chunk = view.get<Chunk>(entity);
        ChunkMesh& chunkMesh = view.get<ChunkMesh>(entity);

        for (auto offset : Chunk::Neighbors26) {
            entt::entity neighborEntity = chunk.neighbor(offset);

            if (m_world->valid(neighborEntity)) {
                auto& neighbor = view.get<Chunk>(neighborEntity);
                neighborChunks[root + offset] = &neighbor;
            } else {
                neighborChunks[root + offset] = nullptr;
            }
        }

        for (int32_t x = -1; x < Chunk::chunkSize + 1; x++) {
            for (int32_t y = -1; y < Chunk::chunkSize + 1; y++) {
                for (int32_t z = -1; z < Chunk::chunkSize + 1; z++) {
                    glm::ivec3 pos = { x, y, z };
                    auto results = Chunk::split(pos);
                    glm::ivec3 chunkOffset = results[0];
                    glm::ivec3 posMod = results[1];

                    if (chunkOffset == glm::ivec3()) {
                        blocks[root + pos] = chunk.blocks()[posMod];
                        light[root + pos] = chunk.light()[posMod];
                    } else {
                        auto neighborChunk = neighborChunks[root + chunkOffset];

                        if (neighborChunk != nullptr) {
                            blocks[root + pos] = neighborChunk->blocks()[posMod];
                            light[root + pos] = neighborChunk->light()[posMod];
                        } else {
                            blocks[root + pos] = Block(0);
                            light[root + pos] = Light(15);
                        }
                    }
                }
            }
        }

        auto& lightUpdates = chunk.getLightUpdates();

        while (lightUpdates.size() > 0) {
            auto update = lightUpdates.front();
            lightUpdates.pop();
            queue.push(update);
        }
    }

    size_t updateIndex = makeMesh(worldChunkPos, blocks, light);
    m_resultQueue.enqueue({ updateIndex, worldChunkPos, light });
}

size_t ChunkMesher::makeMesh(glm::ivec3 worldChunkPos, ChunkBuffer& chunkBuffer, LightBuffer& lightBuffer) {
    size_t index = m_updateIndex;
    m_updateIndex = (m_updateIndex + 1) % (queueSize * 2);

    MeshUpdate& update = m_updates[index];
    update.vertexData.clear();
    update.colorData.clear();
    update.uvData.clear();

    uint32_t indexCount = 0;
    const glm::ivec3 root = { 1, 1, 1 };

    auto view = m_world->registry().view<Chunk>();

    for (glm::ivec3 pos : Chunk::Positions()) {
        Block block = chunkBuffer[root + pos];
        if (block.type == 0) continue;
        if (block.type == 1) continue;
        BlockType& blockType = m_blockManager->getType(block.type);

        ChunkData<Block, 3> neighborBlocks;
        ChunkData<Light, 3> neighborLight;

        for (auto offset : Chunk::Neighbors26) {
            neighborBlocks[root + offset] = chunkBuffer[root + pos + offset];
            neighborLight[root + offset] = lightBuffer[root + pos + offset];
        }

        for (size_t i = 0; i < Chunk::Neighbors6.size(); i++) {
            glm::ivec3 offset = Chunk::Neighbors6[i];
            glm::ivec3 neighborPos = pos + offset;
            glm::ivec3 worldNeighborPos = neighborPos + (worldChunkPos * Chunk::chunkSize);

            const int32_t worldHeightMin = 0;
            const int32_t worldHeightMax = World::worldHeight * Chunk::chunkSize;

            bool visible = neighborBlocks[root + offset].type == 1 || worldNeighborPos.y >= worldHeightMax || worldNeighborPos.y < worldHeightMin;

            if (visible) {
                const Chunk::FaceData& faceData = Chunk::NeighborFaces[i];
                size_t faceIndex = blockType.getFaceIndex(i);

                for (size_t j = 0; j < faceData.vertices.size(); j++) {
                    int32_t light = neighborLight[root + offset].sun;

                    for (size_t k = 0; k < 3; k++) {
                        light += neighborLight[root + faceData.ambientOcclusion[j][k]].sun;
                    }

                    light /= 4;
                    light = std::max(light * 17, 0);

                    update.vertexData.push_back(glm::i8vec4(pos + faceData.vertices[j], 0));
                    update.colorData.push_back(glm::i8vec4(light, light, light, 0));
                    update.uvData.push_back(glm::i8vec4(Chunk::uvFaces[j], static_cast<uint8_t>(faceIndex), 0));
                }

                indexCount++;
            }
        }
    }

    update.indexCount = indexCount * 6;

    return index;
}

void ChunkMesher::transferMesh(ChunkMesh& chunkMesh, size_t index) {
    MeshUpdate& update = m_updates[index];

    if (update.indexCount == 0) {
        chunkMesh.clearMesh();
        return;
    }

    chunkMesh.mesh().setIndexCount(update.indexCount);

    size_t vertexSize = update.vertexData.size() * sizeof(glm::i8vec4);
    size_t colorSize = update.colorData.size() * sizeof(glm::i8vec4);
    size_t uvSize = update.uvData.size() * sizeof(glm::i8vec4);

    if (chunkMesh.mesh().bindingCount() == 0 || chunkMesh.mesh().getBinding(0)->size() < vertexSize) {
        vk::BufferCreateInfo vertexInfo = {};
        vertexInfo.size = vertexSize;
        vertexInfo.usage = vk::BufferUsageFlags::VertexBuffer | vk::BufferUsageFlags::TransferDst;
        vertexInfo.sharingMode = vk::SharingMode::Exclusive;

        vk::BufferCreateInfo colorInfo = vertexInfo;
        colorInfo.size = colorSize;

        vk::BufferCreateInfo uvInfo = vertexInfo;

        VmaAllocationCreateInfo allocInfo = {};
        allocInfo.usage = VMA_MEMORY_USAGE_GPU_ONLY;
        allocInfo.preferredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        std::shared_ptr<VoxelEngine::Buffer> vertexBuffer = std::make_shared<VoxelEngine::Buffer>(*m_engine, vertexInfo, allocInfo);
        std::shared_ptr<VoxelEngine::Buffer> colorBuffer = std::make_shared<VoxelEngine::Buffer>(*m_engine, colorInfo, allocInfo);
        std::shared_ptr<VoxelEngine::Buffer> uvBuffer = std::make_shared<VoxelEngine::Buffer>(*m_engine, uvInfo, allocInfo);

        chunkMesh.mesh().clearBindings();
        chunkMesh.mesh().addBinding(vertexBuffer);
        chunkMesh.mesh().addBinding(colorBuffer);
        chunkMesh.mesh().addBinding(uvBuffer);

        chunkMesh.mesh().setIndexBuffer(m_meshManager->indexBuffer(), vk::IndexType::Uint32, 0);
    }

    m_transferNode->transfer(*chunkMesh.mesh().getBinding(0), vertexSize, 0, update.vertexData.data());
    m_transferNode->transfer(*chunkMesh.mesh().getBinding(1), colorSize, 0, update.colorData.data());
    m_transferNode->transfer(*chunkMesh.mesh().getBinding(2), uvSize, 0, update.uvData.data());

    chunkMesh.setDirty();
}