
#include <drivers/keyboard.h>

using namespace myos::common;
using namespace myos::drivers;
using namespace myos::hardwarecommunication;


KeyboardEventHandler::KeyboardEventHandler()
{
}

void KeyboardEventHandler::OnKeyDown(char, bool)
{
}

void KeyboardEventHandler::OnKeyUp(char, bool)
{
}





KeyboardDriver::KeyboardDriver(InterruptManager* manager, KeyboardEventHandler *handler)
: InterruptHandler(manager, 0x21),
dataport(0x60),
commandport(0x64)
{
    this->handler = handler;
}

KeyboardDriver::~KeyboardDriver()
{
}

void printf(char*);
void printfHex(uint8_t);

void KeyboardDriver::Activate()
{
    while(commandport.Read() & 0x1)
        dataport.Read();
    commandport.Write(0xae); // activate interrupts
    commandport.Write(0x20); // command 0x20 = read controller command byte
    uint8_t status = (dataport.Read() | 1) & ~0x10;
    commandport.Write(0x60); // command 0x60 = set controller command byte
    dataport.Write(status);
    dataport.Write(0xf4);
}


bool isLeftShiftPressed;
bool isRightShiftPressed;

uint32_t KeyboardDriver::HandleInterrupt(uint32_t esp)
{
    uint8_t key = dataport.Read();
    
    if(handler == 0)
        return esp;

    if (key == 0x2A) {
        isLeftShiftPressed = true;
    } else if (key == (0x2A + 0x80)) {
        isLeftShiftPressed = false;
    } else if (key == 0x36) {
        isRightShiftPressed = true;
    } else if (key == (0x36 + 0x80)) {
        isRightShiftPressed = false;
    }
    
    if(key < 0x80)
    {
        switch(key)
        {
            case 0x02:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('!', false); break;
                } else {
                    handler->OnKeyDown('1', false); break;
                }

            case 0x03:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('@', false); break;
                } else {
                    handler->OnKeyDown('2', false); break;
                }

            case 0x04:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('#', false); break;
                } else {
                    handler->OnKeyDown('3', false); break;
                }

            case 0x05:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('$', false); break;
                } else {
                    handler->OnKeyDown('4', false); break;
                }

            case 0x06:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('%', false); break;
                } else {
                    handler->OnKeyDown('5', false); break;
                }

            case 0x07:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('^', false); break;
                } else {
                    handler->OnKeyDown('6', false); break;
                }

            case 0x08:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('&', false); break;
                } else {
                    handler->OnKeyDown('7', false); break;
                }

            case 0x09:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('*', false); break;
                } else {
                    handler->OnKeyDown('8', false); break;
                }

            case 0x0A:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('(', false); break;
                } else {
                    handler->OnKeyDown('9', false); break;
                }

            case 0x0B:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown(')', false); break;
                } else {
                    handler->OnKeyDown('0', false); break;
                }

            case 0x0C:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('_', false); break;
                } else {
                    handler->OnKeyDown('-', false); break;
                }

            case 0x10: handler->OnKeyDown('q', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x11: handler->OnKeyDown('w', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x12: handler->OnKeyDown('e', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x13: handler->OnKeyDown('r', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x14: handler->OnKeyDown('t', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x15: handler->OnKeyDown('y', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x16: handler->OnKeyDown('u', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x17: handler->OnKeyDown('i', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x18: handler->OnKeyDown('o', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x19: handler->OnKeyDown('p', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x1A:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('{', false); break;
                } else {
                    handler->OnKeyDown('[', false); break;
                }
                break;
            case 0x1B:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('}', false); break;
                } else {
                    handler->OnKeyDown(']', false); break;
                }
                break;
            case 0x1E: handler->OnKeyDown('a', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x1F: handler->OnKeyDown('s', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x20: handler->OnKeyDown('d', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x21: handler->OnKeyDown('f', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x22: handler->OnKeyDown('g', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x23: handler->OnKeyDown('h', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x24: handler->OnKeyDown('j', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x25: handler->OnKeyDown('k', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x26: handler->OnKeyDown('l', isLeftShiftPressed | isRightShiftPressed); break;

            case 0x2C: handler->OnKeyDown('z', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x2D: handler->OnKeyDown('x', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x2E: handler->OnKeyDown('c', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x2F: handler->OnKeyDown('v', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x30: handler->OnKeyDown('b', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x31: handler->OnKeyDown('n', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x32: handler->OnKeyDown('m', isLeftShiftPressed | isRightShiftPressed); break;
            case 0x33:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('<', false); break;
                } else {
                    handler->OnKeyDown(',', false); break;
                }
                break;
            case 0x34:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('>', false); break;
                } else {
                    handler->OnKeyDown('.', false); break;
                }
                break;
            case 0x35:
                if(isLeftShiftPressed | isRightShiftPressed) {
                    handler->OnKeyDown('?', false); break;
                } else {
                    handler->OnKeyDown('/', false); break;
                }
                break;

            case 0x1C: handler->OnKeyDown('\n', false); break;
            case 0x0E: handler->OnKeyDown('\b', false); break;
            case 0x39: handler->OnKeyDown(' ', false); break;

            default:
            {
                //printf("KEYBOARD 0x");
                //printfHex(key);
                break;
            }
        }
    }
    return esp;
}
