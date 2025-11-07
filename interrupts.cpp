/**
 *
 * @file interrupts.cpp
 * 
 *
 */

#include<interrupts.hpp>

std::tuple<std::string, std::string, int> simulate_trace(std::vector<std::string> trace_file, int time, std::vector<std::string> vectors, std::vector<int> delays, std::vector<external_file> external_files, PCB current, std::vector<PCB> wait_queue, int &next_pid) {

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

            execution += std::to_string(current_time) + ", " + std::to_string(delays[duration_intr]) + ", SYSCALL ISR: handle syscall (validate args, perform service)\n";
            current_time += delays[duration_intr];

            execution +=  std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;
        } else if(activity == "END_IO") {
            auto [intr, time] = intr_boilerplate(current_time, duration_intr, 10, vectors);
            current_time = time;
            execution += intr;

            execution += std::to_string(current_time) + ", " + std::to_string(delays[duration_intr]) + ", ENDIO ISR: complete I/O, wake waiting process (if any)\n";
            current_time += delays[duration_intr];

            // If there is a process waiting (blocked on I/O), wake it and schedule
            if(!wait_queue.empty()) {
                // wake the first waiting process (FCFS)
                PCB awakened = wait_queue.front();
                wait_queue.erase(wait_queue.begin());

                // Log that the process was woken and a snapshot of the system
                system_status += "time: " + std::to_string(current_time) + "; current trace: END_IO, " + std::to_string(duration_intr) + "\n";
                system_status += print_PCB(awakened, wait_queue) + "\n";

                // Scheduler called: choose awakened process to run (simplified)
                execution += std::to_string(current_time) + ", 0, scheduler called (wake process PID " + std::to_string(awakened.PID) + ")\n";
                execution += std::to_string(current_time) + ", 1, IRET\n";
                current_time += 1;

                // make awakened the current process for subsequent trace processing
                current = awakened;
            } else {
                // No waiting processes
                execution += std::to_string(current_time) + ", 0, scheduler called (no waiting process)\n";
                execution += std::to_string(current_time) + ", 1, IRET\n";
                current_time += 1;
            }
        } else if(activity == "FORK") {
            auto [intr, time] = intr_boilerplate(current_time, 2, 10, vectors);
            execution += intr;
            current_time = time;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //Add your FORK output here

            // Log FORK ISR simulation (after intr_boilerplate)
            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", cloning the PCB\n";
            current_time += duration_intr;

            // Create child PCB (copy parent, assign new PID)
            PCB child(next_pid++, current.PID, current.program_name, current.size, -1);

            // Parent becomes waiting (simulated block) and child is scheduled. Attempt to allocate memory for child.
            wait_queue.push_back(current); // parent is now waiting

            bool child_allocated = allocate_memory(&child);

            // Log snapshot of system after FORK, showing child and wait queue
            system_status += "time: " + std::to_string(current_time) + "; current trace: FORK, " + std::to_string(duration_intr) + "\n";
            if(child_allocated) {
                system_status += print_PCB(child, wait_queue) + "\n";
            } else {
                // Child couldn't be allocated: mark as not in memory (partition -1)
                system_status += "Child PID " + std::to_string(child.PID) + " allocation failed - no partition available\n";
                system_status += print_PCB(child, wait_queue) + "\n";
            }
            
            // Scheduler called and returned
            execution += std::to_string(current_time) + ", 0, scheduler called\n";
            execution += std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;

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

            if (!child_trace.empty()) {
                auto [child_exec, child_status, child_end_time] =
                    simulate_trace(child_trace, current_time, vectors, delays, external_files, child, wait_queue, next_pid);
            
                execution += child_exec;
                system_status += child_status;
                current_time = child_end_time;
            }

            ///////////////////////////////////////////////////////////////////////////////////////////

        } else if(activity == "EXEC") {
            auto [intr, time] = intr_boilerplate(current_time, 3, 10, vectors);
            current_time = time;
            execution += intr;

            ///////////////////////////////////////////////////////////////////////////////////////////
            //Add your EXEC output here

            // Simulate EXEC ISR
            unsigned int prog_size = get_size(program_name, external_files);
            if (prog_size == (unsigned int)-1) {
                execution += std::to_string(current_time) + ", 0, ERROR: Program not found (" + program_name + ")\n";
                break;
            }

            execution += std::to_string(current_time) + ", " + std::to_string(duration_intr) + ", Program is " + std::to_string(prog_size) + " Mb large\n";
            current_time += duration_intr;

            // Simulate loader time (15ms per MB)
            int load_time = prog_size * 15;
            execution += std::to_string(current_time) + ", " + std::to_string(load_time) + ", loading program into memory\n";
            current_time += load_time;
            
            // Free old memory, allocate new one
            free_memory(&current);
            current.program_name = program_name;
            current.size = prog_size;
            allocate_memory(&current);
            
            // Log updates
            execution += std::to_string(current_time) + ", 3, marking partition as occupied\n";
            current_time += 3;
            execution += std::to_string(current_time) + ", 6, updating PCB\n";
            current_time += 6;
            
            // Log system snapshot
            system_status += "time: " + std::to_string(current_time) + "; current trace: EXEC " + program_name + ", " + std::to_string(duration_intr) + "\n";
            system_status += print_PCB(current, wait_queue) + "\n";
            
            // Scheduler + return
            execution += std::to_string(current_time) + ", 0, scheduler called\n";
            execution += std::to_string(current_time) + ", 1, IRET\n";
            current_time += 1;
            
            ///////////////////////////////////////////////////////////////////////////////////////////


            std::ifstream exec_trace_file(program_name + ".txt");

            std::vector<std::string> exec_traces;
            std::string exec_trace;
            while(std::getline(exec_trace_file, exec_trace)) {
                exec_traces.push_back(exec_trace);
            }

            ///////////////////////////////////////////////////////////////////////////////////////////
            //With the exec's trace (i.e. trace of external program), run the exec (HINT: think recursion)

            if (!exec_traces.empty()) {
                auto [exec_exec, exec_status, exec_end_time] =
                    simulate_trace(exec_traces, current_time, vectors, delays, external_files, current, wait_queue, next_pid);
            
                execution += exec_exec;
                system_status += exec_status;
                current_time = exec_end_time;
            }



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

    int next_pid = 1; // to assign unique PIDs for children
    
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
                                            wait_queue,
                                            next_pid);

    input_file.close();

    write_output(execution, "execution.txt");
    write_output(system_status, "system_status.txt");

    return 0;
}
