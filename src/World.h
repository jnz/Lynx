#pragma once

class CWorld;
#include <map>
#ifdef __GNUC__
#include <ext/hash_map>
#else
#include <hash_map>
#endif
#include <list>
#include "Obj.h"
#include "BSPLevel.h"
#include "ResourceManager.h"

/*
    CWorld ist der wichtigste Kern in der Lynx Engine.
    Der Zustand einer Welt wird Datenmäßig in der 
    world_state_t Struktur festgehalten und von
    der CWorld Klasse verwaltet.

    Update() aktualisiert die Welt bei jedem Frame
    Mit Serialize() kann die Welt als Datenstrom abgespeichert werden bzw. über das Netzwerk verschickt werden

 */

/*
    world_state_t fasst die internen Daten von CWorld zusammen und ist nach außen hin nicht interessant
    world_state_t speichert alle Daten über die Welt ab, inklusive ihrer Objekte.
    Intern repräsentiert world_state_t damit den Zustand der Welt zu einem bestimmten Zeitpunkt.

    if you change world_state_t, change:
    - GenerateWorldState
    - Serialize
 */

#ifdef __GNUC__
namespace stdext
{
    using namespace __gnu_cxx;
}
#endif

#define WORLD_STATE_OBJMAPTYPE      stdext::hash_map<int, int>
#define WORLD_STATE_OBJITER         stdext::hash_map<int, int>::iterator
#define WORLD_STATE_CONSTOBJITER    stdext::hash_map<int, int>::const_iterator

struct world_state_t
{
    uint32_t   leveltime; // in ms
    uint32_t   worldid; // fortlaufende nummer
    std::string level; // Pfad zu Level

    // Im Prinzip ist nur der objstate Vektor interessant, aber um schnellen
    // Zugriff auf die Objekt-States mit Hilfe einer Hash-Tabelle zu ermöglichen,
    // sind folgende Hilfsfunktionen nützlich, bzw. erforderlich (früher war objstates public)
    void        AddObjState(obj_state_t objstate, const int id);
    bool        ObjStateExists(const int id) const;
    bool        GetObjState(const int id, obj_state_t& objstate) const;
    WORLD_STATE_OBJITER ObjBegin() { return objindex.begin(); }
    WORLD_STATE_OBJITER ObjEnd() { return objindex.end(); }
    int         GetObjCount() const { return (int)objstates.size(); }

protected:
    std::vector<obj_state_t> objstates; // Liste mit allen Objekten
    WORLD_STATE_OBJMAPTYPE objindex; // ID zu objstates Index Tabelle. Key = obj id, Value = Index in objstates Tabelle
};

struct world_obj_trace_t // Zum Suchen von Objekten die von Strahl getroffen werden
{
    // Input
    vec3_t  start; // start point
    vec3_t  dir; // end point = start + dir
    int     excludeobj; // welche objekt wird vom strahl ignoriert

    // Output
    float   f; // impact = start + f*dir
    vec3_t  hitpoint;
    vec3_t  hitnormal;
    int     objid;
};

// Diese beiden Macros geben an, wie CObj von CWorld verwaltet wird (welcher STL map Typ)
#define OBJMAPTYPE   stdext::hash_map<int, CObj*>
#define OBJITER      stdext::hash_map<int, CObj*>::iterator
#define OBJITERCONST stdext::hash_map<int, CObj*>::const_iterator

class CWorld
{
public:
    CWorld(void);
    virtual ~CWorld(void);

    virtual bool IsClient() const { return false; } // Hilfsfunktion, um bestimmte Aktionen für einen Server nicht auszuführen (Texturen laden)

    virtual void Update(const float dt, const uint32_t ticks); // Neues Frame berechnen

    void    AddObj(CObj* obj, bool inthisframe=false); // Objekt in Welt hinzufügen. Speicher wird automatisch von World freigegeben
    void    DelObj(int objid); // Objekt aus Welt entfernen. Wird beim nächsten Frame gelöscht

    CObj*   GetObj(int objid); // Objekt mit dieser ID suchen

    int     GetObjCount() const { return (int)m_objlist.size(); } // Anzahl der Objekte in Welt
    OBJITER ObjBegin() { return m_objlist.begin(); } // Begin Iterator
    OBJITER ObjEnd() { return m_objlist.end(); } // End Iterator

    const std::vector<CObj*> GetNearObj(const vec3_t& origin, const float radius, const int exclude, const int type) const; // List is only valid for this frame!

    virtual bool Serialize(bool write, CStream* stream, const world_state_t* oldstate=NULL); // Komplette Welt in einen Byte-Stream schreiben. true, wenn sich welt gegenüber oldstate verändert hat

    bool    LoadLevel(const std::string path); // Level laden und BSP Tree vorbereiten
    const virtual CBSPLevel* GetBSP() const { return &m_bsptree; }
    uint32_t   GetLeveltime() const { return state.leveltime; } // Levelzeit, beginnt bei 0
    uint32_t   GetWorldID() const { return state.worldid; } // WorldID erhöht sich bei jedem Update() aufruf um 1

    virtual CResourceManager* GetResourceManager() { return &m_resman; }

    void    ObjMove(CObj* obj, const float dt) const; // Objekt bewegen + Kollisionserkennung

    bool    TraceObj(world_obj_trace_t* trace);

    world_state_t GetWorldState();

protected:
    CResourceManager m_resman;
    world_state_t state;
    uint32_t    m_leveltimestart;
    CBSPLevel m_bsptree;

    OBJMAPTYPE m_objlist;
    void    UpdatePendingObjs(); // Entfernt zu löschende Objekte und fügt neue hinzu (siehe m_addobj und removeobj)
    void    DeleteAllObjs(); // Löscht alle Objekte aus m_objlist und gibt auch den Speicher frei

    std::list<CObj*> m_addobj; // Liste von Objekten die im nächsten Frame hinzugefügt werden
    std::list<int> m_removeobj; // Liste von Objekten die im nächsten Frame gelöscht werden
};

