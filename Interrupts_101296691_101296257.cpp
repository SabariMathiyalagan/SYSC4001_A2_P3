/**
 *
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @author Sabari Mathiyalagan 101296257
 * @author George Tzemenakis 101296691
 *
 */

#include<interrupts.hpp>
static unsigned int NEXT_PID = 1;
std::tuple<std::string, std::string, int> simulate_trace(std::vector<std::string> trace_file, int time, std::vector<std::string> vectors, std::vector<int> delays, std::vector<external_file> external_files, PCB current, std::vector<PCB> wait_queue) {

    std::string trace;      //!< string to store single line of trace file
    std::string execution = "";  //!< string to accumulate the execution output
    std::string system_status = "";  //!< string to accumulate the system status output
    int current_time = time;

    //parse each line of the input trace file. 'for' loop to keep track of indices.
    for(size_t i = 0; i < trace_file.size(); i++) {
        auto trace = trace_file[i];

        auto [activity, duration_intr, program_name] = parse_trace(trace);

        if(activity == "CPU") { //As per Assignment 1
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", CPU Burst\n";
            current_time += duration_intr;
        } else if(activity == "SYSCALL") { //As per Assignment 1
            auto [intr, time] = intr_boilerplate(current_time, duration_intr, 10, vectors);
            execution += intr;
            current_time = time;

            execution += std::to_string(current_time) + ", " + std::to_string(delays[duration_intr]) + ", SYSCALL ISR (ADD STEPS HERE)\n";
            current_time += delays[duration_intr];

            execution +=  std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;
        } else if(activity == "END_IO") {
            auto [intr, time] = intr_boilerplate(current_time, duration_intr, 10, vectors);
            current_time = time;
            execution += intr;

            execution += std::to_string(current_time) + ", " + std::to_string(delays[duration_intr]) + ", ENDIO ISR(ADD STEPS HERE)\n";
            current_time += delays[duration_intr];

            execution +=  std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;
        } else if(activity == "FORK") {
            auto [intr, time] = intr_boilerplate(current_time, 2, 10, vectors);
            execution += intr;
            current_time = time;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //Add your FORK output here
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", cloning the PCB\n";
            current_time += duration_intr;

           
            execution += std::to_string(current_time) + ", 0, scheduler called\n";
            execution += std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;

            PCB child(NEXT_PID++, current.PID, current.program_name, current.size, -1);
            if (!allocate_memory(&child)) {
                std::cerr << "ERROR! Memory allocation failed for child!\n";
            }
            wait_queue.push_back(current);

            std::string current_line = trace_file[i];
            system_status += "time: " + std::to_string(current_time) + "; current trace: " + current_line + "\n";
            system_status += print_PCB(child, wait_queue);



            ///////////////////////////////////////////////////////////////////////////////////////////

            //The following loop helps you do 2 things:
            // * Collect the trace of the chile (and only the child, skip parent)
            // * Get the index of where the parent is supposed to start executing from
            std::vector<std::string> child_trace;
            bool skip = true;
            bool exec_flag = false;
            int parent_index = 0;

            for(size_t j = i; j < trace_file.size(); j++) {
                auto [_activity, _duration, _pn] = parse_trace(trace_file[j]);
                if(skip && _activity == "IF_CHILD") {
                    skip = false;
                    continue;
                } else if(_activity == "IF_PARENT"){
                    skip = true;
                    parent_index = j;
                    if(exec_flag) {
                        break;
                    }
                } else if(skip && _activity == "ENDIF") {
                    skip = false;
                    continue;
                } else if(!skip && _activity == "EXEC") {
                    skip = true;
                    child_trace.push_back(trace_file[j]);
                    exec_flag = true;
                }

                if(!skip) {
                    child_trace.push_back(trace_file[j]);
                }
            }
            i = parent_index;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //With the child's trace, run the child (HINT: think recursion)
            // Run CHILD first (recursively) with child_trace
            auto [exec_child, status_child, time_child] =
            simulate_trace(child_trace, current_time, vectors, delays, external_files, child, wait_queue);

            execution      += exec_child;
            system_status  += status_child;
            current_time    = time_child;

           
            if (child.partition_number != -1) {
                    free_memory(&child);
                }

               
                if (!wait_queue.empty()) wait_queue.pop_back();


            ///////////////////////////////////////////////////////////////////////////////////////////


        } else if(activity == "EXEC") {
            auto [intr, time] = intr_boilerplate(current_time, 3, 10, vectors);
            current_time = time;
            execution += intr;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //Add your EXEC output here
            unsigned int prog_size = get_size(program_name, external_files);

           
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", Program is "
                    + std::to_string(prog_size) + " Mb large\n";
            current_time += duration_intr;

           
            int load_ms = static_cast<int>(prog_size) * 15;
            execution += std::to_string(current_time) + ", " + std::to_string(load_ms) + ", loading program into memory\n";
            current_time += load_ms;

            execution += std::to_string(current_time) + ", 3, marking partition as occupied\n";
            current_time += 3;

            execution += std::to_string(current_time) + ", 6, updating PCB\n";
            current_time += 6;

         
            if(current.partition_number != -1) {
                free_memory(&current);
            }
            current.program_name = program_name;
            current.size         = prog_size;
            allocate_memory(&current);

       
            execution += std::to_string(current_time) + ", 0, scheduler called\n";
            execution += std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;

       
            std::string current_line = trace_file[i];
            system_status += "time: " + std::to_string(current_time) + "; current trace: " + current_line + "\n";
            system_status += print_PCB(current, wait_queue);



            ///////////////////////////////////////////////////////////////////////////////////////////


            std::ifstream exec_trace_file(program_name + ".txt");

            std::vector<std::string> exec_traces;
            std::string exec_trace;
            while(std::getline(exec_trace_file, exec_trace)) {
                exec_traces.push_back(exec_trace);
            }

            ///////////////////////////////////////////////////////////////////////////////////////////
            //With the exec's trace (i.e. trace of external program), run the exec (HINT: think recursion)
            // Now execute the external program trace in this SAME (current) process
            auto [exec_prog, status_prog, time_prog] =
            simulate_trace(exec_traces, current_time, vectors, delays, external_files, current, wait_queue);

            execution      += exec_prog;
            system_status  += status_prog;
            current_time    = time_prog;



            ///////////////////////////////////////////////////////////////////////////////////////////

            break; //Why is this important? (answer in report)

        }
    }

    return {execution, system_status, current_time};
}

int main(int argc, char** argv) {

    //vectors is a C++ std::vector of strings that contain the address of the ISR
    //delays  is a C++ std::vector of ints that contain the delays of each device
    //the index of these elemens is the device number, starting from 0
    //external_files is a C++ std::vector of the struct 'external_file'. Check the struct in
    //interrupt.hpp to know more.
    auto [vectors, delays, external_files] = parse_args(argc, argv);
    std::ifstream input_file(argv[1]);

    //Just a sanity check to know what files you have
    print_external_files(external_files);

    //Make initial PCB (notice how partition is not assigned yet)
    PCB current(0, -1, "init", 1, -1);
    //Update memory (partition is assigned here, you must implement this function)
    if(!allocate_memory(&current)) {
        std::cerr << "ERROR! Memory allocation failed!" << std::endl;
    }

    std::vector<PCB> wait_queue;

    /******************ADD YOUR VARIABLES HERE*************************/


    /******************************************************************/

    //Converting the trace file into a vector of strings.
    std::vector<std::string> trace_file;
    std::string trace;
    while(std::getline(input_file, trace)) {
        trace_file.push_back(trace);
    }

    auto [execution, system_status, _] = simulate_trace(   trace_file,
                                            0,
                                            vectors,
                                            delays,
                                            external_files,
                                            current,
                                            wait_queue);

    input_file.close();

    write_output(execution, "output_files/execution.txt");
    write_output(system_status, "output_files/system_status.txt");


    return 0;
}