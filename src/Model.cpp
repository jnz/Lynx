#include "Model.h"


CModel::CModel()
{

}

CModel::~CModel()
{

}

// This is a registry to map the animation names to unique animation ids
struct anim_registry_t
{
    animation_t id;
    std::string name;
    animation_t next_anim; // if this animation is over, what should (normally) be played next
};

static const anim_registry_t g_anim_registry[] =
{
    {ANIMATION_NONE   , "NONE"   , ANIMATION_NONE},
    {ANIMATION_IDLE   , "idle"   , ANIMATION_IDLE},
    {ANIMATION_RUN    , "run"    , ANIMATION_RUN},
    {ANIMATION_ATTACK , "attack" , ANIMATION_ATTACK},
    {ANIMATION_FIRE   , "fire"   , ANIMATION_FIRE}
};
const int g_anim_count = sizeof(g_anim_registry)/sizeof(g_anim_registry[0]);

animation_t CModel::GetAnimationFromString(const std::string& animation_name)
{
    for(int i=0;i<g_anim_count;i++)
    {
        if(g_anim_registry[i].name == animation_name)
            return g_anim_registry[i].id;
    }
    fprintf(stderr, "Animation: %s not found\n", animation_name.c_str());
    return ANIMATION_NONE;
}

std::string CModel::GetStringFromAnimation(animation_t animation)
{
    assert(animation < ANIMATION_COUNT);
    assert(animation >= 0);

    for(int i=0;i<g_anim_count;i++)
    {
        if(g_anim_registry[i].id == animation)
            return g_anim_registry[i].name;
    }
    fprintf(stderr, "Animation: %i not found\n", animation);
    return "Not found in g_anim_registry";
}

animation_t CModel::GetNextAnimation(const animation_t animation)
{
    assert(animation < ANIMATION_COUNT);
    assert(animation >= 0);
    assert(g_anim_registry[animation].id == animation); // the order in g_anim_registry does not match the animation_t enum

    return g_anim_registry[animation].next_anim;
}

