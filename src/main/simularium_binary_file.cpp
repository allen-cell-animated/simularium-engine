#include "simularium/fileio/simularium_binary_file.h"
#include "loguru/loguru.hpp"

namespace aics {
namespace simularium {
    namespace fileio {

        void SimulariumBinaryFile::Create(std::string filePath, std::size_t numFrames)
        {
            if (this->m_fstream) {
                this->m_fstream.close();
            }

            LOG_F(INFO, "Creating new simularium binary file at %s", filePath.c_str());

            this->m_fstream.open(
                filePath.c_str(),
                std::ios_base::binary | std::ios_base::in | std::ios_base::out | std::ios_base::trunc);

            this->WriteHeader();
            this->AllocateTOC(numFrames);
        }

        void SimulariumBinaryFile::Open(std::string filePath)
        {
            if (this->m_fstream) {
                this->m_fstream.close();
            }

            LOG_F(INFO, "Opening existing simularium binary file at %s", filePath.c_str());

            this->m_fstream.open(
                filePath.c_str(),
                std::ios_base::binary | std::ios_base::in | std::ios_base::out);
        }

        void SimulariumBinaryFile::WriteFrame(TrajectoryFrame frame)
        {
            if (!this->m_fstream) {
                LOG_F(WARNING, "No file opened. Call SimulariumBinaryFile.Create([filepath])");
                return;
            }

            std::vector<float> frameChunk;

            frameChunk.push_back(float(frame.frameNumber));
            frameChunk.push_back(frame.time);
            frameChunk.push_back(float(frame.data.size()));

            for (auto& agentData : frame.data) {
                frameChunk.push_back(agentData.vis_type);
                frameChunk.push_back(agentData.id);
                frameChunk.push_back(agentData.type);
                frameChunk.push_back(agentData.x);
                frameChunk.push_back(agentData.y);
                frameChunk.push_back(agentData.z);
                frameChunk.push_back(agentData.xrot);
                frameChunk.push_back(agentData.yrot);
                frameChunk.push_back(agentData.zrot);
                frameChunk.push_back(agentData.collision_radius);
                frameChunk.push_back(agentData.subpoints.size());

                for (std::size_t j = 0; j < agentData.subpoints.size(); ++j) {
                    frameChunk.push_back(agentData.subpoints[j]);
                }
            }

            // Read the current number of frames saved to the file
            this->m_fstream.seekg(fileio::binary::TOC_ENTRY_COUNT_OFFSET, std::ios_base::beg);
            int nFrames;
            this->m_fstream.read((char*)&nFrames, sizeof(nFrames));

            // Get the stream position of the new frame chunk
            this->m_fstream.seekg(0, std::ios_base::end);
            int framePos = int(this->m_fstream.tellg());

            // Save the frame-chunk stream position in the offset look-up
            int tocPos = fileio::binary::TOC_ENTRY_START_OFFSET + nFrames * 4;
            this->m_fstream.seekp(tocPos, std::ios_base::beg);
            this->m_fstream.write((char*)&framePos, sizeof(framePos));

            // Save the frame chunk data out
            this->m_fstream.seekp(0, std::ios_base::end);
            this->m_fstream.write((char*)&frameChunk[0], frameChunk.size() * sizeof(frameChunk[0]));
            this->m_fstream.write((char*)fileio::binary::eof, sizeof(unsigned char) * 20);

            // Update the number of frames loaded in the file
            nFrames++;
            this->m_fstream.seekp(fileio::binary::TOC_ENTRY_COUNT_OFFSET, std::ios_base::beg);
            this->m_fstream.write((char*)&nFrames, sizeof(nFrames));
        }

        void SimulariumBinaryFile::WriteHeader()
        {
            if (!this->m_fstream) {
                LOG_F(WARNING, "No file opened. Call SimulariumBinaryFile.Create([filepath])");
                return;
            }

            std::vector<unsigned char> header({ 'S', 'I', 'M', 'U', 'L', 'A', 'R', 'I', 'U', 'M', 'B', 'I', 'N' });

            header.push_back(char(1)); // major version
            header.push_back(char(0)); // minor version
            header.push_back(char(0)); // patch version

            this->m_fstream.seekp(0, std::ios_base::beg);
            this->m_fstream.write((char*)&header[0], header.size() * sizeof(header[0]));
        }

        void SimulariumBinaryFile::AllocateTOC(std::size_t size)
        {
            if (!this->m_fstream) {
                LOG_F(WARNING, "No file opened. Call SimulariumBinaryFile.Create([filepath])");
                return;
            }

            std::vector<int> tocChunk(size + 1, 0);

            this->m_fstream.seekp(fileio::binary::HEADER_SIZE, std::ios_base::beg);
            this->m_fstream.write((char*)&tocChunk[0], tocChunk.size() * sizeof(tocChunk[0]));

            this->m_fstream.seekg(0, std::ios_base::end);
            this->m_endTOC = std::size_t(this->m_fstream.tellg());
        }

        std::size_t SimulariumBinaryFile::NumSavedFrames()
        {
            if (!this->m_fstream || !this->m_fstream.good()) {
                LOG_F(WARNING, "fstream is invalid, resetting...");
                this->m_fstream.clear();
                this->m_fstream.seekg(0, std::ios_base::beg);
                this->m_fstream.seekp(0, std::ios_base::beg);
            }

            this->m_fstream.seekg(fileio::binary::TOC_ENTRY_COUNT_OFFSET, std::ios_base::beg);
            int nFrames;
            this->m_fstream.read((char*)&nFrames, sizeof(nFrames));
            return nFrames;
        }

        BroadcastUpdate SimulariumBinaryFile::GetBroadcastFrame(
            std::size_t frameNumber)
        {
            auto numFrames = this->NumSavedFrames();
            if (frameNumber >= numFrames) {
                LOG_F(WARNING, "Frame %zu requested when there are only %zu saved", frameNumber, numFrames);
                return BroadcastUpdate();
            }

            bool isLastFrame = (frameNumber == (numFrames - 1));

            // Get the stored offset for the frame from the 'table of contents' block
            int tocPos = fileio::binary::TOC_ENTRY_START_OFFSET + frameNumber * 4;
            this->m_fstream.seekg(tocPos, std::ios_base::beg);
            int frameStart;
            this->m_fstream.read((char*)&frameStart, sizeof(frameStart));

            int frameEnd; // start of the next frame
            if (isLastFrame) {
                frameEnd = this->GetEndOfFilePos(); // if yes, read to end of file
            } else {
                this->m_fstream.seekg(tocPos + 4, std::ios_base::beg);
                this->m_fstream.read((char*)&frameEnd, sizeof(frameEnd));
            }

            this->m_fstream.seekg(frameStart, std::ios_base::beg);

            BroadcastUpdate out;
            out.buffer.resize((frameEnd - frameStart) / 4);
            this->m_fstream.read(
                reinterpret_cast<char*>(out.buffer.data()), out.buffer.size() * sizeof(out.buffer[0]));

            out.new_pos = frameEnd;

            return out;
        }

        BroadcastUpdate SimulariumBinaryFile::GetBroadcastUpdate(
            std::size_t currentPos,
            std::size_t bufferSize)
        {
            auto start = std::max(currentPos, this->m_endTOC);
            this->m_fstream.seekg(start, std::ios_base::beg);

            BroadcastUpdate out;
            out.buffer.resize(bufferSize / 4);
            this->m_fstream.read(
                reinterpret_cast<char*>(out.buffer.data()), out.buffer.size() * sizeof(out.buffer[0]));

            out.new_pos = start + bufferSize;

            return out;
        }

        std::size_t SimulariumBinaryFile::GetEndOfFilePos()
        {
            this->m_fstream.seekg(0, std::ios_base::end);
            return std::size_t(this->m_fstream.tellg());
        }

        std::size_t SimulariumBinaryFile::GetFramePos(
            std::size_t frameNumber)
        {
            int tocPos = fileio::binary::TOC_ENTRY_START_OFFSET + frameNumber * 4;
            this->m_fstream.seekg(tocPos, std::ios_base::beg);
            int framePos;
            this->m_fstream.read((char*)&framePos, sizeof(framePos));

            return framePos;
        }

    } // namespace fileio
} // namespace simularium
} // namespace aics
