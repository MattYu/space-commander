#include <cstdio>
#include <cstdlib>
#include <time.h>
#include <cstring>
#include <signal.h>
#include <sstream>
#include <unistd.h>
#include <inttypes.h>

#include "Net2Com.h"
#include "command-factory.h"
#include "shakespeare.h"
#include "subsystems.h"
#include "SpaceDecl.h"

const string LAST_COMMAND_FILENAME("last-command");
const int COMMAND_RESEND_INDEX = 0;
const char COMMAND_RESEND_CHAR = '!';
const int MAX_COMMAND_SIZE     = 255;

const char ERROR_CREATING_COMMAND  = '1';
const char ERROR_EXECUTING_COMMAND = '2';

// Declarations
static void out_of_memory_handler();
static int perform(int bytes);


static char info_buffer[255] = {'\0'};
static Net2Com* commander = 0; 

const char* LOGNAME = cs1_systems[CS1_COMMANDER];
/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * NAME : main 
 *
 * DESCRIPTION : space-commander main 
 *
 *-----------------------------------------------------------------------------*/
int main() 
{
    set_new_handler(&out_of_memory_handler);


    commander = new Net2Com(Dcom_w_net_r, Dnet_w_com_r, 
                                                    Icom_w_net_r, Inet_w_com_r);

    if (!commander) {
        fprintf(stderr, "[ERROR] %s:%s:%d Failed in Net2Com instanciation\n", 
                                                __FILE__, __func__, __LINE__);
        return EXIT_FAILURE; /* watch-puppy will take care of 
                              * restarting space-commander
                              */
    }

    Shakespeare::log(Shakespeare::NOTICE, LOGNAME, 
                                            "Waiting commands from ground...");

    while (true) 
    {
        memset(info_buffer, 0, sizeof(char) * 255);
        int bytes = commander->ReadFromInfoPipe(info_buffer, 255);

        if (bytes > 0) {
            perform(bytes);
        }

        sleep(COMMANER_SLEEP_TIME);
    }

    if (commander) {
        delete commander;
        commander = 0;
    }

    return 0;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * NAME : perform 
 *
 * DESCRIPTION : reads from the pipes and execute the command.
 *
 *-----------------------------------------------------------------------------*/
int perform(int bytes)
{
    char* buffer = NULL;    // TODO  This buffer scared me ! 
    int read_total = 0;
    ICommand* command  = NULL;
    char previous_command_buffer[MAX_COMMAND_SIZE] = {'\0'};
    unsigned char read = 0;

    for(int i = 0; i != bytes; i++) {
        read = (unsigned char)info_buffer[i];

        std::ostringstream msg;
        msg << "Read from info pipe = " << (unsigned int)read << " bytes";
        Shakespeare::log(Shakespeare::NOTICE, LOGNAME, msg.str());

        switch (read) 
        {
            case 252: 
                break;
            case 253:
            case 254:
            case 255: 
            {
                int data_bytes = 0;
                while (data_bytes == 0) {
                    data_bytes = commander->ReadFromDataPipe(buffer, read_total);

                    if (data_bytes > 0) {
                        std::ostringstream msg;
                        msg << "Read " << data_bytes << " bytes from ground station: ";
                        Shakespeare::log(Shakespeare::NOTICE, LOGNAME, msg.str());

                        for(uint8_t z = 0; z < data_bytes; ++z){
                            uint8_t c = buffer[z];
                            fprintf(stderr, "0x%02X ", c);
                        }

                        fprintf(stderr, "\n");
                        fflush(stdout);

                        if (data_bytes != read_total) {
                            fprintf(stderr, "Something went wrong !!\n");
                            fflush(stdout);
                            read_total = 0;
                            break;
                        }

                        FILE *fp_last_command = NULL;
                        unsigned int retry = 10000;

                        if (buffer[COMMAND_RESEND_INDEX] == COMMAND_RESEND_CHAR) 
                        {
                            while(retry > 0 && fp_last_command == NULL){
                                fp_last_command = fopen(LAST_COMMAND_FILENAME.c_str(), "r");
                                retry -=1;
                            }

                            if (fp_last_command != NULL) 
                            {
                                fread(previous_command_buffer, sizeof(char), MAX_COMMAND_SIZE, fp_last_command);
                                fclose(fp_last_command);

                                command = CommandFactory::CreateCommand(previous_command_buffer);

                                if (command != NULL) 
                                {
                                    Shakespeare::log(Shakespeare::NOTICE, 
                                                                    LOGNAME, 
                                                                            "Executing command");

                                    char* result  = (char* )command->Execute();
                                    if (result != NULL) {
                                        fprintf(stderr, "Command output = %s\n", result);
                                        fflush(stdout);

                                        commander->WriteToDataPipe(result);
                                        free(result); // TODO allocate result buffer with new in all icommand subclasses and use delete
                                        result = NULL;
                                    } else {
                                        commander->WriteToInfoPipe(ERROR_EXECUTING_COMMAND);
                                    }

                                    if (command) {
                                        delete command;
                                        command = NULL;
                                    }
                                } else {
                                    commander->WriteToInfoPipe(ERROR_CREATING_COMMAND);
                                }

                                memset(previous_command_buffer, '\0', MAX_COMMAND_SIZE);
                            }
                        } else {

                            while (retry > 0 && fp_last_command == NULL) {
                                fp_last_command = fopen(LAST_COMMAND_FILENAME.c_str(), "w");
                                retry -= 1;
                            }

                            if (fp_last_command != NULL) {
                                fwrite(buffer, sizeof(char), data_bytes, fp_last_command);
                                fclose(fp_last_command);
                            }
                        }

                        free(buffer);
                        buffer = NULL;
                    }

                    sleep(COMMANER_SLEEP_TIME);
                } //end while

                read_total = 0;
                break;
            }
            default:
                    read_total += read;
                    buffer = (char* )realloc(buffer, read_total * sizeof(char)); // todo get rid of this realloc! use new instead
                    memset(buffer, 0, sizeof(char) * read_total);
                break;
        } // end switch
    } // end for

    return 0;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * NAME : out_of_memory_handler 
 *
 * DESCRIPTION : This function is called when memory allocation with new fails.
 *
 *-----------------------------------------------------------------------------*/
void out_of_memory_handler()
{
    std::cerr << "[ERROR] new failed\n";
    throw bad_alloc();
}
