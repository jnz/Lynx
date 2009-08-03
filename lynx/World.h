#pragma once

class CWorld;
#include <map>
#include <list>
#include "Obj.h"
#include "BSPTree.h"
#include "ResourceManager.h"

#define OBJMAPTYPE	std::map<int, CObj*>
#define OBJITER		std::map<int, CObj*>::iterator

/*
    if you change world_state_t, change:
    - GenerateWorldState
    - Serialize
 */
struct world_state_t
{
    DWORD   leveltime; // in ms
    DWORD   worldid; // fortlaufende nummer

    std::string level; // Pfad zu Level
    std::vector<obj_state_t> objstates;
    std::map<int,int> objindex; // ID zu objstates index tabelle
};

class CWorld
{
public:
	CWorld(void);
	virtual ~CWorld(void);

    virtual bool IsClient() { return false; }

	virtual void Update(const float dt); // Neues Frame berechnen

	void	AddObj(CObj* obj, bool inthisframe=false); // Objekt in Welt hinzufügen. Speicher wird automatisch von World freigegeben
	void	DelObj(int objid); // Objekt aus Welt entfernen. Wird beim nächsten Frame gelöscht

	CObj*	GetObj(int objid); // Objekt mit dieser ID suchen

	int		GetObjCount() const { return (int)m_objlist.size(); } // Anzahl der Objekte in Welt
	OBJITER ObjBegin() { return m_objlist.begin(); } // Begin Iterator
	OBJITER ObjEnd() { return m_objlist.end(); } // End Iterator

	bool	Serialize(bool write, CStream* stream, const world_state_t* oldstate=NULL); // Komplette Welt in einen Byte-Stream schreiben. true, wenn sich welt gegenüber oldstate verändert hat

    bool    LoadLevel(const std::string path);
    const   CBSPTree* GetBSP() { return &m_bsptree; }
    DWORD   GetLeveltime() const { return state.leveltime; }
    DWORD   GetWorldID() const { return state.worldid; } // WorldID erhöht sich bei jedem Update() aufruf um 1
    world_state_t GenerateWorldState();

    CResourceManager m_resman;

protected:

	world_state_t state;
    DWORD    m_leveltimestart;
    CBSPTree m_bsptree;

	OBJMAPTYPE m_objlist;
	void	UpdatePendingObjs(); // Entfernt zu löschende Objekte und fügt neue hinzu (siehe m_addobj und removeobj)
	void	DeleteAllObjs(); // Löscht alle Objekte aus m_objlist und gibt auch den Speicher frei

	void	ObjCollision(CObj* obj, float dt);

	std::list<CObj*> m_addobj; // Liste von Objekten die im nächsten Frame hinzugefügt werden
	std::list<int> m_removeobj; // Liste von Objekten die im nächsten Frame gelöscht werden
};
