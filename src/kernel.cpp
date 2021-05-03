
#include <common/types.h>
#include <gdt.h>
#include <memorymanagement.h>
#include <hardwarecommunication/interrupts.h>
#include <syscalls.h>
#include <hardwarecommunication/pci.h>
#include <drivers/driver.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/vga.h>
#include <drivers/ata.h>
#include <gui/desktop.h>
#include <gui/window.h>
#include <multitasking.h>

#include <drivers/amd_am79c973.h>
#include <net/etherframe.h>
#include <net/arp.h>
#include <net/ipv4.h>
#include <net/icmp.h>
#include <net/udp.h>
#include <net/tcp.h>


#define NULL ((char *)0)
// #define GRAPHICSMODE

using namespace myos;
using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;
using namespace myos::gui;
using namespace myos::net;

typedef size_t rsize_t;
#define RSIZE_MAX 9223372036854775807

struct kernelGlobal
{
    bool TERMINALMODE;
    unsigned int CommandIndex;
    char CommandBuffer[64];
};

/* OSDEV WIKI (wiki.osdev.org)*/
static inline void outb(uint16_t port, uint8_t val)
{
    asm volatile ( "outb %0, %1" : : "a"(val), "Nd"(port) );
    /* There's an outb %al, $imm8  encoding, for compile-time constant port numbers that fit in 8b.  (N constraint).
     * Wider immediate constants would be truncated at assemble-time (e.g. "i" constraint).
     * The  outb  %al, %dx  encoding is the only option for all other cases.
     * %1 expands to %dx because  port  is a uint16_t.  %w1 could be used if we had the port number a wider C type */
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile ( "inb %1, %0"
                   : "=a"(ret)
                   : "Nd"(port) );
    return ret;
}
/* OSDEV WIKI (wiki.osdev.org)*/

kernelGlobal KernelGlobal = { .TERMINALMODE = false, .CommandIndex = 0};
void printf(char* str)
{
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    static uint8_t x=0,y=0;

    for(int i = 0; str[i] != '\0'; ++i)
    {
        switch(str[i])
        {
            case '\n':
                x = 0;
                y++;
                break;
            case '\b':
                if(KernelGlobal.TERMINALMODE) {
                    if(x > 2) {
                        x = x - 1;
                        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
                    }
                } else if(x > 0) {
                    x = x - 1;
                    VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
                }
                break;
            default:
                VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | str[i];
                x++;
                break;
        }

        if(x >= 80)
        {
            x = 0;
            y++;
        }

        if(y >= 25)
        {
            for(y = 0; y < 25; y++)
                for(x = 0; x < 80; x++)
                    VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
            x = 0;
            y = 0;
        }
    }
}

void clear() {
    static uint16_t* VideoMemory = (uint16_t*)0xb8000;

    uint8_t x=0, y=0;

    for(y = 0; y < 25; y++)
        for(x = 0; x < 80; x++)
            VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0xFF00) | ' ';
    x = 0;
    y = 0;
}

void printfHex(uint8_t key)
{
    char* foo = "00";
    char* hex = "0123456789ABCDEF";
    foo[0] = hex[(key >> 4) & 0xF];
    foo[1] = hex[key & 0xF];
    printf(foo);
}
void printfHex16(uint16_t key)
{
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}
void printfHex32(uint32_t key)
{
    printfHex((key >> 24) & 0xFF);
    printfHex((key >> 16) & 0xFF);
    printfHex((key >> 8) & 0xFF);
    printfHex( key & 0xFF);
}

void * memset( void * s, int c, size_t n )
{
    unsigned char * p = ( unsigned char * ) s;

    while ( n-- )
    {
        *p++ = ( unsigned char ) c;
    }

    return s;
}

void * memcpy( void * s1, const void * s2, size_t n )
{
    char * dest = ( char * ) s1;
    const char * src = ( const char * ) s2;

    while ( n-- )
    {
        *dest++ = *src++;
    }

    return s1;
}

void reboot()
{
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
loop:
    asm("hlt");
    goto loop;
}

void clearArray( char array[], int length ) 
{
    memset( array, 0, ( length * sizeof( char ) )); 
 
    return; 
}

int bufferSize(char array[]) {
    return (sizeof(array)/sizeof(char));
}

int stringSize(char array[]) {
    int size = 0;
    for(int i = 0; array[i] != '\0'; ++i) {
        size++;
    }
    return size;
}


int strcmp( const char * s1, const char * s2 )
{
    while ( ( *s1 ) && ( *s1 == *s2 ) )
    {
        ++s1;
        ++s2;
    }

    return ( *( unsigned char * )s1 - * ( unsigned char * )s2 ) == 0;
}

char * lib_strtok( char * s1, rsize_t *  s1max, const char * s2, char ** ptr )
{
    const char * p = s2;

    if ( s1max == 0 || s2 == NULL || (char*)ptr == NULL || ( s1 == NULL && *ptr == NULL ) || *s1max > RSIZE_MAX )
    {
        return NULL;
    }

    if ( s1 != NULL )
    {
        /* new string */
        *ptr = s1;
    }
    else
    {
        /* old string continued */
        if ( *ptr == NULL )
        {
            /* No old string, no new string, nothing to do */
            return NULL;
        }

        s1 = *ptr;
    }

    /* skip leading s2 characters */
    while ( *p && *s1 )
    {
        if ( *s1 == *p )
        {
            /* found separator; skip and start over */
            if ( *s1max == 0 )
            {
                return NULL;
            }

            ++s1;
            --( *s1max );
            p = s2;
            continue;
        }

        ++p;
    }

    if ( ! *s1 )
    {
        /* no more to parse */
        *ptr = s1;
        return NULL;
    }

    /* skipping non-s2 characters */
    *ptr = s1;

    while ( **ptr )
    {
        p = s2;

        while ( *p )
        {
            if ( **ptr == *p++ )
            {
                /* found separator; overwrite with '\0', position *ptr, return */
                if ( *s1max == 0 )
                {
                    return NULL;
                }

                --( *s1max );
                *( ( *ptr )++ ) = '\0';
                return s1;
            }
        }

        if ( *s1max == 0 )
        {
            return NULL;
        }

        --( *s1max );
        ++( *ptr );
    }

    /* parsed to end of string */
    return s1;
}

size_t strlen( const char * s )
{
    size_t rc = 0;

    while ( s[rc] )
    {
        ++rc;
    }

    return rc;
}

char * strtok( char * s1, const char * s2 )
{
    static char * tmp = NULL;
    static rsize_t max;

    if ( s1 != NULL )
    {
        /* new string */
        tmp = s1;
        max = strlen( tmp );
    }

    return lib_strtok( s1, &max, s2, &tmp );
}

char * splitString(char commandBuffer[]) {
    char * pch;
    char *args[4];
    int index = 0;

    pch = strtok (commandBuffer," ");
    while (pch != NULL)
    {
        printf(pch);
        args[index++] = pch;
        pch = strtok (NULL, " ");
    }
    return *args;
}

void executeCommand(char* args[], char* rawArgs) {
    if(strcmp(args[0], "test")) {
        printf("If your seeing this message than the command handler is most likely working\n");
    } else if(strcmp(args[0], "reboot")) {
        reboot();
    } else if(strcmp(args[0], "echo")) {
        int init_size = strlen(rawArgs);
        char delim[] = " ";

        char *ptr = strtok(rawArgs, delim);
        ptr = strtok(NULL, delim);

        while(ptr != NULL)
        {
            printf(ptr);
            printf(" ");
            ptr = strtok(NULL, delim);
        }
        printf("\n");
    }
}

void parseCommand(char commandBuffer[]) {
    char commandBuffer2[64];
    memcpy(commandBuffer2, commandBuffer, strlen(commandBuffer)+1);
    char *args[16];
    int index = 0;

    int init_size = strlen(commandBuffer);
	char delim[] = " ";

	char *ptr = strtok(commandBuffer, delim);

	while(ptr != NULL)
	{
        args[index++] = ptr;
		ptr = strtok(NULL, delim);
	}
    executeCommand(args, commandBuffer2);
}

class PrintfKeyboardEventHandler : public KeyboardEventHandler
{
public:
    void OnKeyDown(char c, bool uppercase)
    {
        if(uppercase)
            c = c - 32;
        char* foo = " ";
        foo[0] = c;
        printf(foo);
        if(c == '\b') {
            if(stringSize(KernelGlobal.CommandBuffer) > 0)
                KernelGlobal.CommandBuffer[KernelGlobal.CommandIndex--] = '\0';

        } else if(c == '\n') {
            parseCommand(KernelGlobal.CommandBuffer);
            clearArray(KernelGlobal.CommandBuffer, 64);
            KernelGlobal.CommandIndex = 0;
            printf("$ ");
        } else {
            KernelGlobal.CommandBuffer[KernelGlobal.CommandIndex++] = c;
        }
    }
};

class MouseToConsole : public MouseEventHandler
{
    int8_t x, y;
public:
    
    MouseToConsole()
    {
        uint16_t* VideoMemory = (uint16_t*)0xb8000;
        x = 40;
        y = 12;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);        
    }
    
    virtual void OnMouseMove(int xoffset, int yoffset)
    {
        static uint16_t* VideoMemory = (uint16_t*)0xb8000;
        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);

        x += xoffset;
        if(x >= 80) x = 79;
        if(x < 0) x = 0;
        y += yoffset;
        if(y >= 25) y = 24;
        if(y < 0) y = 0;

        VideoMemory[80*y+x] = (VideoMemory[80*y+x] & 0x0F00) << 4
                            | (VideoMemory[80*y+x] & 0xF000) >> 4
                            | (VideoMemory[80*y+x] & 0x00FF);
    }
    
};

class PrintfUDPHandler : public UserDatagramProtocolHandler
{
public:
    void HandleUserDatagramProtocolMessage(UserDatagramProtocolSocket* socket, common::uint8_t* data, common::uint16_t size)
    {
        char* foo = " ";
        for(int i = 0; i < size; i++)
        {
            foo[0] = data[i];
            printf(foo);
        }
    }
};


class PrintfTCPHandler : public TransmissionControlProtocolHandler
{
public:
    bool HandleTransmissionControlProtocolMessage(TransmissionControlProtocolSocket* socket, common::uint8_t* data, common::uint16_t size)
    {
        char* foo = " ";
        for(int i = 0; i < size; i++)
        {
            foo[0] = data[i];
            //printf(foo);
        }
        
        
        
        if(size > 9
            && data[0] == 'G'
            && data[1] == 'E'
            && data[2] == 'T'
            && data[3] == ' '
            && data[4] == '/'
            && data[5] == ' '
            && data[6] == 'H'
            && data[7] == 'T'
            && data[8] == 'T'
            && data[9] == 'P'
        )
        {
            socket->Send((uint8_t*)"HTTP/1.1 200 OK\r\nServer: ACULOS\r\nContent-Type: text/html\r\n\r\n<html><head><title>Test Page</title></head><body><p>If your seeing this then </p><b>It Works!</b></body></html>\r\n",173);
            socket->Disconnect();
        }
        
        
        return true;
    }
};



void sysprintf(char* str)
{
    asm("int $0x80" : : "a" (4), "b" (str));
}

void taskA()
{
    while(true)
        sysprintf("A");
}

void taskB()
{
    while(true)
        sysprintf("B");
}


void disable_cursor()
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

typedef void (*constructor)();
extern "C" constructor start_ctors;
extern "C" constructor end_ctors;
extern "C" void callConstructors()
{
    for(constructor* i = &start_ctors; i != &end_ctors; i++)
        (*i)();
}



extern "C" void kernelMain(const void* multiboot_structure, uint32_t /*multiboot_magic*/)
{
    printf("ACULOS Kernel 2021.1 Unstable\n");

    GlobalDescriptorTable gdt;
    
    
    uint32_t* memupper = (uint32_t*)(((size_t)multiboot_structure) + 8);
    size_t heap = 10*1024*1024;
    MemoryManager memoryManager(heap, (*memupper)*1024 - heap - 10*1024);
    
    printf("heap: 0x");
    printfHex((heap >> 24) & 0xFF);
    printfHex((heap >> 16) & 0xFF);
    printfHex((heap >> 8 ) & 0xFF);
    printfHex((heap      ) & 0xFF);
    
    void* allocated = memoryManager.malloc(1024);
    printf("\nallocated: 0x");
    printfHex(((size_t)allocated >> 24) & 0xFF);
    printfHex(((size_t)allocated >> 16) & 0xFF);
    printfHex(((size_t)allocated >> 8 ) & 0xFF);
    printfHex(((size_t)allocated      ) & 0xFF);
    printf("\n");
    
    TaskManager taskManager;
    /*
    Task task1(&gdt, taskA);
    Task task2(&gdt, taskB);
    taskManager.AddTask(&task1);
    taskManager.AddTask(&task2);
    */
    
    InterruptManager interrupts(0x20, &gdt, &taskManager);
    SyscallHandler syscalls(&interrupts, 0x80);


    // disable cursor (because its annoying)
    disable_cursor();
    
    printf("Initializing Hardware, Stage 1\n");
    
    #ifdef GRAPHICSMODE
        Desktop desktop(320,200, 0x00,0x00,0xA8);
    #endif
    
    DriverManager drvManager;
    
        #ifdef GRAPHICSMODE
            KeyboardDriver keyboard(&interrupts, &desktop);
        #else
            PrintfKeyboardEventHandler kbhandler;
            KeyboardDriver keyboard(&interrupts, &kbhandler);
        #endif
        drvManager.AddDriver(&keyboard);
        
    
        #ifdef GRAPHICSMODE
            MouseDriver mouse(&interrupts, &desktop);
        #else
            MouseToConsole mousehandler;
            MouseDriver mouse(&interrupts, &mousehandler);
        #endif
        drvManager.AddDriver(&mouse);
        
        PeripheralComponentInterconnectController PCIController;
        PCIController.SelectDrivers(&drvManager, &interrupts);

        #ifdef GRAPHICSMODE
            VideoGraphicsArray vga;
        #endif
        
    printf("Initializing Hardware, Stage 2\n");
        drvManager.ActivateAll();
        
    printf("Initializing Hardware, Stage 3\n");

    #ifdef GRAPHICSMODE
        vga.SetMode(320,200,8);
        Window win1(&desktop, 10,10,20,20, 0xA8,0x00,0x00);
        desktop.AddChild(&win1);
        Window win2(&desktop, 40,15,30,30, 0x00,0xA8,0x00);
        desktop.AddChild(&win2);
    #endif


    /*
    printf("\nS-ATA primary master: ");
    AdvancedTechnologyAttachment ata0m(true, 0x1F0);
    ata0m.Identify();
    
    printf("\nS-ATA primary slave: ");
    AdvancedTechnologyAttachment ata0s(false, 0x1F0);
    ata0s.Identify();
    ata0s.Write28(0, (uint8_t*)"http://www.AlgorithMan.de", 25);
    ata0s.Flush();
    ata0s.Read28(0, 25);
    
    printf("\nS-ATA secondary master: ");
    AdvancedTechnologyAttachment ata1m(true, 0x170);
    ata1m.Identify();
    
    printf("\nS-ATA secondary slave: ");
    AdvancedTechnologyAttachment ata1s(false, 0x170);
    ata1s.Identify();
    // third: 0x1E8
    // fourth: 0x168
    */
    

                 

                   
    amd_am79c973* eth0 = (amd_am79c973*)(drvManager.drivers[2]);

    
    // IP Address
    uint8_t ip1 = 10, ip2 = 0, ip3 = 2, ip4 = 15;
    uint32_t ip_be = ((uint32_t)ip4 << 24)
                | ((uint32_t)ip3 << 16)
                | ((uint32_t)ip2 << 8)
                | (uint32_t)ip1;
    eth0->SetIPAddress(ip_be);
    EtherFrameProvider etherframe(eth0);
    AddressResolutionProtocol arp(&etherframe);    

    
    // IP Address of the default gateway
    uint8_t gip1 = 10, gip2 = 0, gip3 = 2, gip4 = 2;
    uint32_t gip_be = ((uint32_t)gip4 << 24)
                   | ((uint32_t)gip3 << 16)
                   | ((uint32_t)gip2 << 8)
                   | (uint32_t)gip1;
    
    uint8_t subnet1 = 255, subnet2 = 255, subnet3 = 255, subnet4 = 0;
    uint32_t subnet_be = ((uint32_t)subnet4 << 24)
                   | ((uint32_t)subnet3 << 16)
                   | ((uint32_t)subnet2 << 8)
                   | (uint32_t)subnet1;
                   
    InternetProtocolProvider ipv4(&etherframe, &arp, gip_be, subnet_be);
    InternetControlMessageProtocol icmp(&ipv4);
    UserDatagramProtocolProvider udp(&ipv4);
    TransmissionControlProtocolProvider tcp(&ipv4);
    
    
    interrupts.Activate();

    printf("\n\n\n\n");
    
    arp.BroadcastMACAddress(gip_be);
    
    
    PrintfTCPHandler tcphandler;
    TransmissionControlProtocolSocket* tcpsocket = tcp.Listen(1234);
    tcp.Bind(tcpsocket, &tcphandler);
    //tcpsocket->Send((uint8_t*)"Hello TCP!", 10);

    
    //icmp.RequestEchoReply(gip_be);
    
    //PrintfUDPHandler udphandler;
    //UserDatagramProtocolSocket* udpsocket = udp.Connect(gip_be, 1234);
    //udp.Bind(udpsocket, &udphandler);
    //udpsocket->Send((uint8_t*)"Hello UDP!", 10);
    
    //UserDatagramProtocolSocket* udpsocket = udp.Listen(1234);
    //udp.Bind(udpsocket, &udphandler);

    
    // MOVE INTO MAGIC MAN

    printf(" \n \n \n \n \n");
    printf("                                     \n");
    printf(" _____ _____ _____ __    _____ _____ \n");
    printf("|  _  |     |  |  |  |  |     |   __|\n");
    printf("|     |   --|  |  |  |__|  |  |__   |\n");
    printf("|__|__|_____|_____|_____|_____|_____|\n");
    printf("                                     \n");
    printf("$ ");
    KernelGlobal.TERMINALMODE = true;


    while(1)
    {
        #ifdef GRAPHICSMODE
            desktop.Draw(&vga);
        #endif

    }
}
