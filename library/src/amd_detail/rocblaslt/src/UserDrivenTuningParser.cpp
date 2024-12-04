#include "UserDrivenTuningParser.hpp"
#include <fstream>
#include <shared_mutex>
#include <sstream>
#include <utility>

namespace TensileLite
{

    void getContractionProblemsFromFile(const std::string& path)
    {
        OverrideMap&                m_override = OverrideMap::getMap();
        std::mutex&                 map_guard  = m_override.getLock();
        std::lock_guard<std::mutex> lock(map_guard);

        if(m_override.size() == 0)
        {

            std::ifstream file_read(path);
            std::string   line, entry;

            const auto verion      = "Git Version";
            const auto delim       = ',';
            const int  max_entries = 37;

            while(std::getline(file_read, line))
            {
                // Ignore lines without delimiter
                line.erase(0, line.find_first_not_of(" \t\n\r\f\v"));

                if(line.find(delim) != std::string::npos && line.find(verion) == std::string::npos)
                {
                    std::vector<std::string> entries{};
                    entries.reserve(max_entries);

                    std::stringstream line_ss(line);
                    while(getline(line_ss, entry, delim))
                    {
                        entries.push_back(entry);
                    }

                    auto problemSolution = problemFromEntries(entries);

                    if(problemSolution.second > 0)
                    {
                        auto sol_iter = m_override.find(problemSolution.first);
                        for(auto sol_idx = sol_iter.first; sol_idx != sol_iter.second; sol_idx++)
                        {
                            if(sol_idx->second == problemSolution.second)
                            {
                                m_override.erase(sol_idx);
                                break;
                            }
                        }

                        m_override.add(problemSolution);
                    }
                }
            }
        }
    }

    std::pair<ProblemOverride, int> problemFromEntries(const std::vector<std::string>& entries)
    {

        const size_t entries_n = entries.size();
        if(entries_n != 37)
        {
            return std::make_pair(ProblemOverride{}, -1);
        }

        //Expected format: transA,transB,batch_count,M,N,K,input_type,output_type,compute_type,solution_index
        bool transA = (entries[0] != "N");
        bool transB = (entries[1] != "N");

        size_t   m, n, b, k;
        DataType inputType   = DataType::None;
        DataType outputType  = DataType::None;
        DataType computeType = DataType::None;

        int solution_idx = -1;

        try
        {

            // TODO: are any additional mapping parameters needed?

            b            = std::stol(entries[3]);
            m            = std::stol(entries[4]);
            n            = std::stol(entries[5]);
            k            = std::stol(entries[6]);
            inputType    = hipDataType_to_tensile_type(string_to_hip_datatype(entries[17]));
            outputType   = hipDataType_to_tensile_type(string_to_hip_datatype(entries[19]));
            computeType  = hipDataType_to_tensile_type(string_to_hip_datatype(entries[21]));
            solution_idx = std::stoi(entries[34]);
        }
        catch(std::invalid_argument const& ex)
        {
            return std::make_pair(ProblemOverride{}, -1);
        }
        catch(std::out_of_range const& ex)
        {
            return std::make_pair(ProblemOverride{}, -1);
        }

        if(inputType == DataType::None || outputType == DataType::None
           || computeType == DataType::None)
        {
            return std::make_pair(ProblemOverride{}, -1);
        }

        ProblemOverride po(transA, transB, inputType, computeType, outputType, m, n, k, b);

        return std::make_pair(po, solution_idx);
    }

    ProblemOverride::ProblemOverride()
        : m_transA(false)
        , m_transB(false)
        , m_inputType(DataType::None)
        , m_computeType(DataType::None)
        , m_outputType(DataType::None)
        , m_m(0)
        , m_n(0)
        , m_k(0)
        , m_batchSize(0)
    {
    }

    ProblemOverride::ProblemOverride(bool     transA,
                                     bool     transB,
                                     DataType inputType,
                                     DataType computeType,
                                     DataType outputType,
                                     size_t   m,
                                     size_t   n,
                                     size_t   k,
                                     size_t   batchSize)
        : m_transA(transA)
        , m_transB(transB)
        , m_inputType(inputType)
        , m_computeType(computeType)
        , m_outputType(outputType)
        , m_m(m)
        , m_n(n)
        , m_k(k)
        , m_batchSize(batchSize)
    {
    }

    ProblemOverride::ProblemOverride(const ProblemOverride& problem)
    {

        m_transA      = problem.transA();
        m_transB      = problem.transB();
        m_inputType   = problem.inputType();
        m_computeType = problem.computeType();
        m_outputType  = problem.outputType();
        m_m           = problem.m();
        m_n           = problem.n();
        m_k           = problem.k();
        m_batchSize   = problem.batchSize();
    }

};
