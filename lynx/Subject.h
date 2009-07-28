#pragma once

#include <list>
#include "Observer.h"

class CSubject
{
public:
	void AddObserver(CObserver* observer);
	void RemoveObserver(CObserver* observer);
	void RemoveAllObserver();

	void NotifyError(int error);

private:
	std::list<CObserver*> m_list;
};
