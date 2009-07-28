#include "lynx.h"
#include "Subject.h"

void CSubject::NotifyError(int error)
{
	fprintf(stderr, "World NotifyError: %i\n", error);
	assert(0);

	std::list<CObserver*>::iterator iter;
	for(iter=m_list.begin();iter!=m_list.end();iter++)
		(*iter)->NotifyError(error);
}

void CSubject::AddObserver(CObserver* observer)
{
	std::list<CObserver*>::iterator iter;

	for(iter=m_list.begin();iter!=m_list.end();iter++)
	{
		if((*iter)==observer)
		{
			assert(0); // already in list
			return;
		}
	}

	m_list.push_back(observer);
}

void CSubject::RemoveObserver(CObserver* observer)
{
	std::list<CObserver*>::iterator iter;

	for(iter=m_list.begin();iter!=m_list.end();iter++)
	{
		if((*iter)==observer)
		{
			m_list.erase(iter);
			return;
		}
	}
	
	assert(0); // observer not found
}

void CSubject::RemoveAllObserver()
{
	m_list.clear();
}