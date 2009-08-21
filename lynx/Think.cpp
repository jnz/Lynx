#include "Think.h"

#ifdef _DEBUG
#include <crtdbg.h>
#define new new(_NORMAL_BLOCK,__FILE__, __LINE__)
#endif

CThink::~CThink()
{
    RemoveAll();
}

void CThink::AddFunc(CThinkFunc* func)
{
    m_think.push_back(func);
}

void CThink::RemoveAll()
{
    std::list<CThinkFunc*>::iterator iter;
    for(iter = m_think.begin(); iter != m_think.end(); iter++)
        delete (*iter);
    m_think.clear();       
}

void CThink::DoThink(DWORD leveltime)
{
    CThinkFunc* func;
    std::list<CThinkFunc*>::iterator iter;
    for(iter = m_think.begin(); iter != m_think.end();)
    {
        func = (*m_think.begin());
        if(func->GetThinktime() <= leveltime)
        {
            if(func->DoThink(leveltime))
            {
                iter = m_think.erase(iter);
                delete func;
                func = NULL;
            }
            else
            {
                iter++;
                assert(func->GetThinktime() >= leveltime);
            }
        }
        else
        {
            iter++;
        }
    }
}