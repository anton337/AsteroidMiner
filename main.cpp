#include <pthread.h>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <iostream>
#include <math.h>
#include <vector>
#include <string.h>
#include <map>
#include <set>
#include <queue>
#include <stack>
#include <algorithm>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <unistd.h>
#include "screenshot.h"
#include "FractalNoise.h"

using namespace std;

bool signal_Jupiter_texture = false;

GLfloat asteroid_m[16];

GLfloat cameraRotation[16];
      
GLfloat xPosition = 0.0
      , yPosition = 70.0
      , zPosition = 0.0
      ;
 
GLfloat xVelocity = 0
      , yVelocity = 0
      , zVelocity = 0
      ;
 
GLfloat xVelocity_prev = 0
      , yVelocity_prev = 0
      , zVelocity_prev = 0
      ;


double mouse_delta_x = 0;
double mouse_delta_y = 0;

int tool_selection = 33;

enum { MINING_TOOL_TYPE = 32
     , WEAPON_TYPE      = 33
     };

string* string_ptr;

class Item
{

    public:

    int type;
    int quantity;
    double volume;
    string name;
    int texture;

    Item(){

    }   

    Item( int _type
        , int _volume
        , string _name
        , int _texture
        )
    {

        type = _type;
        quantity = 0;
        volume = _volume;
        name = _name;
        texture = _texture;

    }
 
};


void draw_text(float x,float y,float z,string str){

    glPushMatrix();
    glTranslatef(x,y,z);
    glRasterPos3f(0,0,0);
    for(int i=0;i<str.size();i++)
    glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,str[i]);
    glPopMatrix();

}

class Signal{
    public:
    virtual void execute(void* arg) = 0;
};

class MenuInput;
class MenuOutput;
struct Block;

class MenuComponent{
    public:
    map<string,Signal*> signal;
    vector<MenuComponent*> component;
    float x_min, x_max;
    float y_min, y_max;
    bool picked;
    std::string name;
    Block* block_parent;
    virtual MenuComponent* pick(float x,float y) = 0;
    virtual void display() = 0;
    virtual void draw() = 0;
    virtual void unselect() = 0;
    virtual void update(void* arg,int i,int n) = 0;
};

class Menu: public MenuComponent{
    public:
    int texture;
    vector<MenuInput*> input;
    vector<MenuOutput*> output;
    std::map<std::string,std::vector<Block*> > vec;
    Menu(){
        picked = false;
        block_parent = NULL;
    }

    void display(){

        glBegin(GL_QUADS);
        float su = .125*(texture%8), eu=su+.125, sv=.125*(texture/8), ev=sv+.125;
        glTexCoord2d(su,sv);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_max,-1);
        glTexCoord2d(eu,sv);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_max,-1);
        glTexCoord2d(eu,ev);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_min,-1);
        glTexCoord2d(su,ev);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_min,-1);
        glEnd();

    }
    
    MenuComponent* pick(float x,float y){
        for(int i=0;i<component.size();i++){
            MenuComponent* c = (MenuComponent*)component[i]->pick(x,y);
            if(c)
                return c;
        }
        if( x >= x_min
         && x <= x_max
         && y >= y_min
         && y <= y_max
          )
        {
            return this;
        }
        return NULL;
    }
    
    void draw(){
        display();
        for(int i=0;i<component.size();i++)
            component[i]->draw();
    }
     
    void unselect(){
        picked = false;
        for(int i=0;i<component.size();i++)
            component[i]->unselect();
    }
    
    void update(void* arg,int i,int n){


    }

};

class Button: public MenuComponent{
    public:
    int texture;
    string label;
    MenuComponent* parent;
    Button(){
        parent = NULL;
        picked = false;
        block_parent = NULL;
    }

    void display(){

        glBegin(GL_QUADS);
        float su = .125*(texture%8), eu=su+.125, sv=.125*(texture/8), ev=sv+.125;
        glTexCoord2d(su,sv);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_max,-1);
        glTexCoord2d(eu,sv);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_max,-1);
        glTexCoord2d(eu,ev);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_min,-1);
        glTexCoord2d(su,ev);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_min,-1);
        glEnd();
        glTexCoord2d(.95,.95);
        draw_text(.75*MenuComponent::x_min+.25*MenuComponent::x_max,.5*(MenuComponent::y_min+MenuComponent::y_max),-1,label);

    }
    
    MenuComponent* pick(float x,float y){
        for(int i=0;i<component.size();i++){
            MenuComponent* c = (MenuComponent*)component[i]->pick(x,y);
            if(c)
                return c;
        }
        if( x >= x_min
         && x <= x_max
         && y >= y_min
         && y <= y_max
          )
        {
            return this;
        }
        return NULL;
    }
    
    void draw(){
        display();
        for(int i=0;i<component.size();i++)
            component[i]->draw();
    }
     
    void unselect(){
        picked = false;
        for(int i=0;i<component.size();i++)
            component[i]->unselect();
    }
    
    void update(void* arg,int i,int n){
        for(int k=0;k<n;k++)
            parent->signal[label]->execute(parent);
    }

};




vector<Item*> item_array;

template<typename T>
class Container {

    public:

    double max_volume;

    double curr_volume;

    vector<T*> v;

    Container(){
        max_volume = 0;
        curr_volume = 0;
    }

    Container(double _max_volume){
        curr_volume = 0;
        max_volume = _max_volume;
    }

    T* find_item(int type){
        for(int i=0;i<v.size();i++)
            if(v[i]->type == type)
                return v[i];
        return NULL;
    }
 
    int get_index(int type){
        for(int i=0;i<v.size();i++)
            if(v[i]->type == type)
                return i;
        return -1;
    }
    
    int insert_item(int type, int quantity = 1){
        //std::cout << "inserting: " << type << std::endl;
        //std::cout << "volume=" << curr_volume << std::endl;
        //for(int i=0;i<v.size();i++)
        //    //std::cout<<"::: v["<<i<<"]->type="<<v[i]->type << std::endl;
        if(curr_volume + item_array[type]->volume*quantity <= max_volume) {
            curr_volume += item_array[type]->volume*quantity;
            T* item = find_item(type);
            if(item){
                //std::cout << "item found" << std::endl;
                item->quantity+=quantity;
            }else{
                //std::cout << "insert " << type << std::endl;
                item = new Item(*item_array[type]);
                item->quantity = quantity;
                v.push_back(item);
            }
            return 0;
        } else {
            //std::cout << "no room in container" << std::endl;
            return 1;
        }
    }
 
    void remove_item(int type, int quantity = 1){
        //std::cout << "removing: " << type << std::endl;
        //std::cout << "volume=" << curr_volume << std::endl;
        //for(int i=0;i<v.size();i++)
        //    //std::cout<<"::: v["<<i<<"]->type="<<v[i]->type << std::endl;
        if(curr_volume > 0) {
            T* item = find_item(type);
            if(item){
                curr_volume -= item_array[type]->volume*quantity;
                //std::cout << "item found" << std::endl;
                item->quantity-=quantity;
                if(item->quantity <= 0){
                    v.erase(v.begin()+get_index(type));
                    delete item;
                }
            }else{
                //std::cout << "item not found" << std::endl;
            }
        } else {
            //std::cout << "container empty" << std::endl;
        }
    }

};

class Player 
{

    public:

    int place_selection;

    float life;
    float total_life;
    float armor;
    float food;
    float oxygen;
    float air;
    float water;
    float radiation;

    float d_food;
    float d_water;
    float d_oxygen;
    float d_life;
   
    Container<Item> storage; 

    Container<Item> tool; 

    Player(){
        place_selection = 0;
    }

    Player(double tool_size, double storage_size){
        tool    = Container<Item>(   tool_size);
        storage = Container<Item>(storage_size);
        total_life = 100;
        life = total_life;
        food = 100;
        water = 100;
        armor = 0;
        oxygen = 100;
        air = 0;
        radiation = 0;

        d_food = 0.0001;
        d_oxygen = 0.001;
        d_water = 0.0002;
        d_life = 0.01;

        place_selection = 0;

    }

    void update() {
        food -= d_food;
        oxygen -= d_oxygen;
        water -= d_water;
        if(food < 0)
            food = 0;
        if(oxygen < 0)
            oxygen = 0;
        if(water < 0)
            water = 0;
        if(food < d_food || oxygen < d_oxygen || water < d_water)
            life -= d_life;
    }

};

class MenuInput: public MenuComponent{
    public:
    Item* item;
    int texture;

    MenuInput(){
        picked = false;
        item = NULL;
        block_parent = NULL;
    }

    void display(){

        glBegin(GL_QUADS);
        float su, eu, sv, ev;
        if(item == NULL){
            su = .125*(texture%8); 
            eu=su+.125; 
            sv=.125*(texture/8); 
            ev=sv+.125;
        } else
        {
            su = .125*(item->texture%8); 
            eu=su+.125; 
            sv=.125*(item->texture/8); 
            ev=sv+.125;
        }
        glTexCoord2d(su,sv);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_max,-1);
        glTexCoord2d(eu,sv);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_max,-1);
        glTexCoord2d(eu,ev);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_min,-1);
        glTexCoord2d(su,ev);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_min,-1);
        glEnd();
        glTexCoord2d(0.95,0.95);
        if(item != NULL){
            stringstream ss;
            ss << item->quantity;
            draw_text(MenuComponent::x_min,MenuComponent::y_min,-1,ss.str());
        }

    }
    
    MenuComponent* pick(float x,float y){
        for(int i=0;i<component.size();i++){
            MenuComponent* c = (MenuComponent*)component[i]->pick(x,y);
            if(c)
                return c;
        }
        if( x >= x_min
         && x <= x_max
         && y >= y_min
         && y <= y_max
          )
        {
            return this;
        }
        return NULL;
    }
    
    void draw(){
        display();
        for(int i=0;i<component.size();i++)
            component[i]->draw();
    }
     
    void unselect(){
        picked = false;
        for(int i=0;i<component.size();i++)
            component[i]->unselect();
    }
     
    void update(void* arg,int update_type,int num){
        Player* p = (Player*)arg;       
        if(p->place_selection < 0)
            p->place_selection += p->tool.v.size();
        for(int i=0;i<num;i++){
            switch(update_type){ 

                case 1:
                {
                    if(p->tool.v.size() > 0)
                    if(item == NULL){
                        item = new Item(*p->tool.v[p->place_selection%p->tool.v.size()]);
                        item->quantity = 1;
                        p->tool.remove_item(p->tool.v[p->place_selection%p->tool.v.size()]->type);
                    }else
                    if(p->tool.v[p->place_selection%p->tool.v.size()]->type == item->type && item->quantity < 64){
                        item->quantity++;
                        p->tool.remove_item(p->tool.v[p->place_selection%p->tool.v.size()]->type);
                    }
                    break;
                }

                case 2:
                {
                    if(item != NULL){
                        if(p->tool.insert_item(item->type) == 0){
                        item->quantity--;
                        if(item->quantity <= 0){
                            delete item;
                            item = NULL;
                        }
                        }
                    }
                    break;
                }

                default:
                {


                }
 
            }
        }
    }



};

bool TEXT     = false;

static int string_count = 1;

class MenuStringInput: public MenuComponent{
    public:
    int texture;
    MenuStringInput(){
        picked = false;
        std::stringstream ss;
        ss << "input" << string_count;
        name = ss.str();
        ++string_count;
        block_parent = NULL;
    }

    void display(){

        glBegin(GL_QUADS);
        float su, eu, sv, ev;
        su = .125*(texture%8); 
        eu=su+.125; 
        sv=.125*(texture/8); 
        ev=sv+.125;
        glTexCoord2d(su,sv);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_max,-1);
        glTexCoord2d(eu,sv);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_max,-1);
        glTexCoord2d(eu,ev);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_min,-1);
        glTexCoord2d(su,ev);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_min,-1);
        glEnd();
        glTexCoord2d(0.95,0.95);
        stringstream ss;
        ss << name;
        draw_text(MenuComponent::x_min,MenuComponent::y_min,-1,ss.str());

    }
    
    MenuComponent* pick(float x,float y){
        for(int i=0;i<component.size();i++){
            MenuComponent* c = (MenuComponent*)component[i]->pick(x,y);
            if(c)
                return c;
        }
        if( x >= x_min
         && x <= x_max
         && y >= y_min
         && y <= y_max
          )
        {
            return this;
        }
        return NULL;
    }
    
    void draw(){
        display();
        for(int i=0;i<component.size();i++)
            component[i]->draw();
    }
     
    void unselect(){
        picked = false;
        for(int i=0;i<component.size();i++)
            component[i]->unselect();
    }
     
    void update(void* arg,int update_type,int num){
        TEXT = true;
        string_ptr = &name;
    }



};

bool NUMERIC_INPUT = false;

static int group_count = 1;

class MenuNumericStringInput: public MenuComponent{
    public:
    int texture;
    MenuNumericStringInput(){
        picked = false;
        std::stringstream ss;
        ss << group_count;
        name = ss.str();
        block_parent = NULL;
    }

    void display(){

        glBegin(GL_QUADS);
        float su, eu, sv, ev;
        su = .125*(texture%8); 
        eu=su+.125; 
        sv=.125*(texture/8); 
        ev=sv+.125;
        glTexCoord2d(su,sv);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_max,-1);
        glTexCoord2d(eu,sv);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_max,-1);
        glTexCoord2d(eu,ev);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_min,-1);
        glTexCoord2d(su,ev);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_min,-1);
        glEnd();
        glTexCoord2d(0.95,0.95);
        stringstream ss;
        ss << name;
        draw_text(MenuComponent::x_min,MenuComponent::y_min,-1,ss.str());

    }
    
    MenuComponent* pick(float x,float y){
        for(int i=0;i<component.size();i++){
            MenuComponent* c = (MenuComponent*)component[i]->pick(x,y);
            if(c)
                return c;
        }
        if( x >= x_min
         && x <= x_max
         && y >= y_min
         && y <= y_max
          )
        {
            return this;
        }
        return NULL;
    }
    
    void draw(){
        display();
        for(int i=0;i<component.size();i++)
            component[i]->draw();
    }
     
    void unselect(){
        picked = false;
        for(int i=0;i<component.size();i++)
            component[i]->unselect();
    }
     
    void update(void* arg,int update_type,int num){
        TEXT = true;
        NUMERIC_INPUT = true;
        string_ptr = &name;
    }



};


class MenuStringOutput: public MenuComponent{
    public:
    int texture;
    MenuStringOutput(){
        picked = false;
        name = "";
        block_parent = NULL;
    }

    void display(){

        glBegin(GL_QUADS);
        float su, eu, sv, ev;
        su = .125*(texture%8); 
        eu=su+.125; 
        sv=.125*(texture/8); 
        ev=sv+.125;
        glTexCoord2d(su,sv);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_max,-1);
        glTexCoord2d(eu,sv);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_max,-1);
        glTexCoord2d(eu,ev);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_min,-1);
        glTexCoord2d(su,ev);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_min,-1);
        glEnd();
        glTexCoord2d(0.95,0.95);
        stringstream ss;
        ss << name;
        draw_text(MenuComponent::x_min,MenuComponent::y_min,-1,ss.str());

    }
    
    MenuComponent* pick(float x,float y){
        for(int i=0;i<component.size();i++){
            MenuComponent* c = (MenuComponent*)component[i]->pick(x,y);
            if(c)
                return c;
        }
        if( x >= x_min
         && x <= x_max
         && y >= y_min
         && y <= y_max
          )
        {
            return this;
        }
        return NULL;
    }
    
    void draw(){
        display();
        for(int i=0;i<component.size();i++)
            component[i]->draw();
    }
     
    void unselect(){
        picked = false;
        for(int i=0;i<component.size();i++)
            component[i]->unselect();
    }
     
    void update(void* arg,int update_type,int num){

    }



};


class MenuOutput: public MenuComponent{
    public:
    Item* item;
    int texture;

    MenuOutput(){
        picked = false;
        item = NULL;
        block_parent = NULL;
    }

    void display(){

        glBegin(GL_QUADS);
        float su, eu, sv, ev;
        if(item == NULL){
            su = .125*(texture%8); 
            eu=su+.125; 
            sv=.125*(texture/8); 
            ev=sv+.125;
        } else
        {
            su = .125*(item->texture%8); 
            eu=su+.125; 
            sv=.125*(item->texture/8); 
            ev=sv+.125;
        }
        glTexCoord2d(su,sv);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_max,-1);
        glTexCoord2d(eu,sv);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_max,-1);
        glTexCoord2d(eu,ev);
        glVertex3f(MenuComponent::x_min,MenuComponent::y_min,-1);
        glTexCoord2d(su,ev);
        glVertex3f(MenuComponent::x_max,MenuComponent::y_min,-1);
        glEnd();
        glTexCoord2d(0.95,0.95);
        if(item != NULL){
            stringstream ss;
            ss << item->quantity;
            draw_text(MenuComponent::x_min,MenuComponent::y_min,-1,ss.str());
        }

    }
    
    MenuComponent* pick(float x,float y){
        for(int i=0;i<component.size();i++){
            MenuComponent* c = (MenuComponent*)component[i]->pick(x,y);
            if(c)
                return c;
        }
        if( x >= x_min
         && x <= x_max
         && y >= y_min
         && y <= y_max
          )
        {
            return this;
        }
        return NULL;
    }
    
    void draw(){
        display();
        for(int i=0;i<component.size();i++)
            component[i]->draw();
    }
     
    void unselect(){
        picked = false;
        for(int i=0;i<component.size();i++)
            component[i]->unselect();
    }
     
    void update(void* arg,int update_type,int num){
        Player* p = (Player*)arg;       
        for(int i=0;i<num;i++){
            switch(update_type){ 

                case 2:
                {
                    if(item != NULL){
                        if(p->tool.insert_item(item->type)==0){
                        item->quantity--;
                        if(item->quantity <= 0){
                            delete item;
                            item = NULL;
                        }
                        }
                    }
                    break;
                }


                default: 
                {

                }

            }
        }
    }



};

/*    
    item_array.push_back( new Item( 2, 1, "rock"                  , 2 ) );
    item_array.push_back( new Item( 3, 1, "iron"                  , 3 ) );
    item_array.push_back( new Item( 4, 1, "ice"                   , 4 ) );
    item_array.push_back( new Item( 5, 1, "uranium"               , 5 ) );
    item_array.push_back( new Item( 6, 1, "carbon"                , 6 ) );
    item_array.push_back( new Item( 7, 1, "hydrocarbon"           , 7 ) );

    item_array.push_back( new Item( 8, 5, "solar panel"           , 8 ) );
    item_array.push_back( new Item( 9, 2, "wire"                  , 9 ) );
    item_array.push_back( new Item(10,10, "capacitor"             ,10 ) );
    item_array.push_back( new Item(11, 5, "lamp"                  ,11 ) );

    item_array.push_back( new Item(12,10, "3D printer"            ,12 ) );
    item_array.push_back( new Item(13,10, "atmosphere generator"  ,13 ) );
    item_array.push_back( new Item(14,10, "container"             ,14 ) );
    item_array.push_back( new Item(15,10, "refinery"              ,15 ) );

    item_array.push_back( new Item(16,10, "engine"                ,16 ) );
    item_array.push_back( new Item(17,10, "control panel"         ,17 ) );
    item_array.push_back( new Item(18,10, "camera"                ,18 ) );
    item_array.push_back( new Item(19,10, "heat dissipator"       ,19 ) );

    item_array.push_back( new Item(20, 2, "stone"                 ,20 ) );
    item_array.push_back( new Item(21, 2, "metal"                 ,21 ) );
    item_array.push_back( new Item(22, 2, "ceramic"               ,22 ) );
    item_array.push_back( new Item(23, 2, "carbon fiber"          ,23 ) );

    item_array.push_back( new Item(24, 1, "water"                 ,24 ) );
    item_array.push_back( new Item(25, 1, "food"                  ,25 ) );
    item_array.push_back( new Item(26,10, "condenser"             ,26 ) );
    item_array.push_back( new Item(27,10, "greenhouse"            ,27 ) );

    item_array.push_back( new Item(28, 1, "window"                ,28 ) );
    item_array.push_back( new Item(29,10, "door"                  ,29 ) );
    item_array.push_back( new Item(30,50, "nuclear generator"     ,30 ) );
    item_array.push_back( new Item(31,10, "heat dissipator"       ,31 ) );
*/

int refine_blueprint(int type){
    switch(type){
        case 2:
            return 20;
        case 3:
            return 21;
        case 1:
            return 22;
        case 6:
            return 23;
        case 4:
            return 24;
        case 22:
            return 28;
        default:
            return 0;
    }
}


int build_blueprint(int type,int& num){
    switch(type){
        case 48427561: // 1 1 1  
                       // 1 1 1  
                       // 1 1 1 
            num = 4;
            return 8;
        case 92065583: case 96788783: // 2 2 2      2 1 2
                                      // 1 1 1      2 1 2
                                      // 2 2 2      2 1 2
            num = 9;   
            return 9;
        case 53270317: // 1 2 1
                       // 2 1 2
                       // 1 2 1
            num = 4;
            return 11;
        case 96855122: // 2 2 2
                       // 2 2 2
                       // 2 2 2
            num = 1;
            return 15;
        case 96842000: // 2 2 2
                       // 2 0 2
                       // 2 2 2
            num = 1;
            return 14;
        case 1016841000: // 21 21 21
                         // 21  0 21
                         // 21 21 21
            num = 1;
            return 12;
        case 145263000: // 3 3 3
                        // 3 0 3
                        // 3 3 3
            num = 1;
            return 10;
        case 1016867244: // 21 21 21
                         // 21  4 21
                         // 21 21 21
            num = 1;
            return 13;
        case 1016926293: // 21 21 21
                         // 21 13 21
                         // 21 21 21
            num = 1;
            return 26;
        case 1065432586: // 22 22 22
                         // 22 26 22
                         // 22 22 22
            num = 1;
            return 27;
        case 1016978781: // 21 21 21
                         // 21 21 21
                         // 21 21 21
            num = 4;
            return 29;
        case 1113715805: // 23 23 23
                         // 23  5 23
                         // 23 23 23
            num = 1;
            return 30;
        case 1065419464: // 22 22 22
                         // 22 24 22
                         // 22 22 22
            num = 1;
            return 31;
        case 1016886927: // 21 21 21
                         // 21  7 21
                         // 21 21 21
            num = 1;
            return 16;
        case 435854610: //  9  9  9
                        //  9 10  9
                        //  9  9  9
            num = 1;
            return 17;
        default:
            num = 1;
            return 0;
    }
}

class BuildSignal: public Signal{
    void execute(void* arg){
        Menu* m = (Menu*)arg;
        if(m->output[0]->item == NULL){
            int type = 0;
            for(int i=0;i<m->input.size();i++)
                if(m->input[i]->item)
                    type += m->input[i]->item->type * pow(m->input.size(),i);
            //std::cout << "type=" << type << std::endl;
            int num = 0;
            if(type > 0 && build_blueprint(type,num) > 0){
                m->output[0]->item = new Item(*item_array[build_blueprint(type,num)]);
                m->output[0]->item->quantity = num;
                for(int i=0;i<m->input.size();i++)
                    if(m->input[i]->item){
                        m->input[i]->item->quantity--;
                        if(m->input[i]->item->quantity <= 0){
                            delete m->input[i]->item;
                            m->input[i]->item = NULL;
                        }
                    }
            }
        }else{
            int type = 0;
            for(int i=0;i<m->input.size();i++)
                if(m->input[i]->item)
                    type += m->input[i]->item->type * pow(m->input.size(),i);
            if(type > 0){
                int num = 0;
                if(build_blueprint(type,num) == m->output[0]->item->type){
                    m->output[0]->item->quantity+=num;
                    for(int i=0;i<m->input.size();i++)
                        if(m->input[i]->item){
                            m->input[i]->item->quantity--;
                            if(m->input[i]->item->quantity <= 0){
                                delete m->input[i]->item;
                                m->input[i]->item = NULL;
                            }
                        }
                }
            }
        }
    }
};

class RefineSignal: public Signal{
    void execute(void* arg){
        Menu* m = (Menu*)arg;
        if(m->output[0]->item == NULL){
            int type = 0;
            for(int i=0;i<m->input.size();i++)
                if(m->input[i]->item)
                    type += m->input[i]->item->type * pow(m->input.size(),i);
            if(type > 0 && refine_blueprint(type) > 0){
                m->output[0]->item = new Item(*item_array[refine_blueprint(type)]);
                m->output[0]->item->quantity = 1;
                for(int i=0;i<m->input.size();i++)
                    if(m->input[i]->item){
                        m->input[i]->item->quantity--;
                        if(m->input[i]->item->quantity <= 0){
                            delete m->input[i]->item;
                            m->input[i]->item = NULL;
                        }
                    }
            }
        }else{
            int type = 0;
            for(int i=0;i<m->input.size();i++)
                if(m->input[i]->item)
                    type += m->input[i]->item->type * pow(m->input.size(),i);
            if(type > 0){
                if(refine_blueprint(type) == m->output[0]->item->type){
                    m->output[0]->item->quantity++;
                    for(int i=0;i<m->input.size();i++)
                        if(m->input[i]->item){
                            m->input[i]->item->quantity--;
                            if(m->input[i]->item->quantity <= 0){
                                delete m->input[i]->item;
                                m->input[i]->item = NULL;
                            }
                        }
                }
            }
        }
    }
};

Player* player;


GLuint texture[11];
GLuint skybox;
GLuint enemy_sphere;
GLuint projectile_sphere;

bool update_scene = false;


class HalfEdge;
class Face;
class Vertex;
class Edge;


class HalfEdge{
  public:
    string name;
    HalfEdge *next, *prev, *flip;
    Face* face;
    Vertex* origin;
    Edge* edge_a;
    Edge* edge_b;
    HalfEdge(){
      next = NULL;
      prev = NULL;
      flip = NULL;
      face = NULL;
      origin = NULL;
      edge_a = NULL;
      edge_b = NULL;
    }

    friend ostream &operator<<(ostream& stream,HalfEdge& obj);


};


class Vector{
  public:
    double p[3];
    Vector(){
      p[0] = 0;
      p[1] = 0;
      p[2] = 0;
    };
    Vector(double x,double y,double z){
      p[0] = x;
      p[1] = y;
      p[2] = z;
    }
    double angle(Vector& vect){
      double dot = p[0]*vect.p[0]+p[1]*vect.p[1]+p[2]*vect.p[2];
      double norm = sqrt(p[0]*p[0]+p[1]*p[1]+p[2]*p[2]);
      double norm1 = sqrt(vect.p[0]*vect.p[0]+vect.p[1]*vect.p[1]+vect.p[2]*vect.p[2]);
      if(norm > 1e-10 && norm1 > 1e-10)
        return acos(dot/(norm*norm1));
      else 
        return 1000;
    }
};

class Face;

class Vertex: public Vector{
  public:
    Face* face;
    string name;
    int index;
    vector<Edge*> out;
    HalfEdge* halfedge;
    Vertex(){
      halfedge = NULL;
      p[0] = 0;
      p[1] = 0;
      p[2] = 0;
    };
    Vertex(double x,double y,double z){
      halfedge = NULL;
      p[0] = x;
      p[1] = y;
      p[2] = z;
    }
    Edge* find(Vertex* v);
    void unique_edges();

    friend ostream &operator<<(ostream& stream,Vertex& obj);

};


class Face{
  public:
    int texture;
    double color_r;
    double color_g;
    double color_b;
    double color_w;
    string name;
    int num;
    int _num;
    HalfEdge* halfedge;
    vector<Vertex*> v;
    vector<Vector> normal;
    GLuint backgroundtexture;
    float* background;
    float* _background;
    int i;
    int j;
    double a,b,c;
    int size;
    bool draw;
    bool init;
    int findVertex(Vertex* u);
    Face(){
      init = true;
      background = NULL;
      num = 50;
      _num = 50;
      size = 0;
      halfedge = NULL;
      draw = false;
    }


    void setnormal(){
      normal.clear();
      double R = 0;
      bool toggle;
      {
        Vector norm;
        Vector v1( v[1]->p[0] - v[0]->p[0]
                 , v[1]->p[1] - v[0]->p[1]
                 , v[1]->p[2] - v[0]->p[2]
                 );
        Vector v2( v[2]->p[0] - v[0]->p[0]
                 , v[2]->p[1] - v[0]->p[1]
                 , v[2]->p[2] - v[0]->p[2]
                 );
        norm.p[0] = v1.p[1]*v2.p[2] - v2.p[1]*v1.p[2];
        norm.p[1] = v1.p[2]*v2.p[0] - v2.p[2]*v1.p[0];
        norm.p[2] = v1.p[0]*v2.p[1] - v2.p[0]*v1.p[1];
        double r = sqrt(norm.p[0]*norm.p[0]+norm.p[1]*norm.p[1]+norm.p[2]*norm.p[2]);
        toggle = false;
        if(r > R){
          R = r;
          toggle = true;
        }
        if(r > .000000000000000001){
          norm.p[0] /= r;
          norm.p[1] /= r;
          norm.p[2] /= r;
          if(normal.size() > 0){
            if(toggle)
              normal[0] = norm;
          }
        }
        normal.push_back(norm);
      }
    }
 


    float area(){
      Vector a(
        v[1]->p[0]-v[0]->p[0],
        v[1]->p[1]-v[0]->p[1],
        v[1]->p[2]-v[0]->p[2]
      );
      Vector b(
        v[2]->p[0]-v[0]->p[0],
        v[2]->p[1]-v[0]->p[1],
        v[2]->p[2]-v[0]->p[2]
      );
      return .5*fabs(a.p[0]*b.p[0] + a.p[1]*b.p[1] + a.p[2]*b.p[2]);
    }


    float bilinear(float* background,float* _background,int i,int j,int w,int _w,int color){

      float u = w*float(i)/_w;
      float v = w*float(j)/_w;
      int x = floor(u);
      int y = floor(v);
      float u_ratio = u - x;
      float v_ratio = v - y;
      float u_opposite = 1 - u_ratio;
      float v_opposite = 1 - v_ratio;
//      cout << x << " " << y << "    " << u_ratio << " " << v_ratio << " " << u_opposite << " " << v_opposite << endl;
      return  
        (background[3*(x*w+y)+color]   * u_opposite  + 
        background[3*((x+1)*w+y)+color]   * u_ratio) * v_opposite + 
        (background[3*(x*w+y+1)+color] * u_opposite  + 
        background[3*((x+1)*w+y+1)+color] * u_ratio) * v_ratio;

    }



    void bilinearFilter(float* background,float* _background,int w,int _w){
      for(int i=0;i<_w;i++)
        for(int j=0;j<_w;j++)
          {
            _background[3*(i*_w+j)+0] = bilinear(background,_background,i,j,w,_w,0);
            _background[3*(i*_w+j)+1] = bilinear(background,_background,i,j,w,_w,1);
            _background[3*(i*_w+j)+2] = bilinear(background,_background,i,j,w,_w,2);
          }
    }

    void setTexCoords(){

      init = false;
      int size = 3*num*num;
      int w = num;
//      int _size = 3*_num*_num;
//      int _w = _num;
      background = new float[size];
//      _background = new float[_size];
//      for(i=0;i<size;i+=3){
//        background[i+0] = 1;
//        background[i+1] = 0;
//        background[i+2] = float(rand()%100)/100;
//      }

//      bilinearFilter(background,_background,w,_w);

//      LoadTextureRAW(_w,_w,_background,false);
//      delete [] background;
//      delete [] _background;
 

//      bilinearFilter(background,_background,w,_w);

      LoadTextureRAW(w,w,background,false);
      delete [] background;
//      delete [] _background;
 
    }


    void setColor(int i,int j,int w,float* background,double (*R)(double,double,double),double (*G)(double,double,double),double (*B)(double,double,double)){
      if(v.size() == 3){
        double x,y,z;
        double a,b,c;
        background[(int)(3*(i*w+j)+0)] = 0;
        background[(int)(3*(i*w+j)+1)] = 0;
        background[(int)(3*(i*w+j)+2)] = 0;
        int I = i;
        int J = j;
        //i = max(0.0,min(w-1.0,double(i)));
        double total_r = 0;
        double total_g = 0;
        double total_b = 0;
        double weight_r;
        double weight_g;
        double weight_b;
        double weight_t;
//        double __A=double(I)/w;
//        double __B=double(J)/w;
        double __A=double(I+.5)/w;
        double __B=double(J+.5)/w;
        double __C=1.0-__A-__B;
        double X = v[0]->p[0]*__A+v[1]->p[0]*__B+v[2]->p[0]*__C;
        double Y = v[0]->p[1]*__A+v[1]->p[1]*__B+v[2]->p[1]*__C;
        double Z = v[0]->p[2]*__A+v[1]->p[2]*__B+v[2]->p[2]*__C;
        double _R = (*R)(X,Y,Z);
        double _G = (*G)(X,Y,Z);
        double _B = (*B)(X,Y,Z);
        double _r,_g,_b;
        float win = 1;
        for(float k=-win;k<=win;k++)
        for(float t=-win;t<=win;t++)
        for(float f=-win;f<=win;f++){
//          a=double(I+k)/w;
//          b=double(J+t)/w;
          a=double(I+.5+k)/w;
          b=double(J+.5+t)/w;
          c=1.0-a-b;
          x = X+.001*k;//v[0]->p[0]*a+v[1]->p[0]*b+v[2]->p[0]*c;
          y = Y+.001*t;//v[0]->p[1]*a+v[1]->p[1]*b+v[2]->p[1]*c;
          z = Z+.001*f;//v[0]->p[2]*a+v[1]->p[2]*b+v[2]->p[2]*c;
          _r = (*R)(x,y,z);
          _g = (*G)(x,y,z);
          _b = (*B)(x,y,z);
          weight_t = 1;//pow(EE,-.5*(pow(X-x,2)+pow(Y-y,2)+pow(Z-z,2))/1);
          weight_r = weight_t;//*pow(EE,-.5*(pow(_R-_r,2))/1);
          weight_g = weight_t;//*pow(EE,-.5*(pow(_G-_g,2))/1);
          weight_b = weight_t;//*pow(EE,-.5*(pow(_B-_b,2))/1);
          total_r += weight_r;
          total_g += weight_g;
          total_b += weight_b;
          background[(int)(3*(i*w+j)+0)] += weight_r*_r;
          background[(int)(3*(i*w+j)+1)] += weight_g*_g;
          background[(int)(3*(i*w+j)+2)] += weight_b*_b;
        }
        background[(int)(3*(i*w+j)+0)] /= total_r;
        background[(int)(3*(i*w+j)+1)] /= total_g;
        background[(int)(3*(i*w+j)+2)] /= total_b;
      }
    }


    void LoadTextureRAW(int width,int height, float* data, int wrap )
    {

      // allocate a texture name
      glGenTextures( 1, &backgroundtexture );

      // select our current texture
      glBindTexture( GL_TEXTURE_2D, backgroundtexture );

      // select modulate to mix texture with color for shading
      glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

      // when texture area is small, bilinear filter the closest mipmap
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                     //GL_LINEAR );
                     //GL_LINEAR_MIPMAP_LINEAR );
                     GL_LINEAR_MIPMAP_NEAREST );
      // when texture area is large, bilinear filter the first mipmap
      //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );
      //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

      // if wrap is true, the texture wraps over at the edges (repeat)
      //       ... false, the texture ends at the edges (clamp)
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                     wrap ? GL_REPEAT : GL_CLAMP );
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                     wrap ? GL_REPEAT : GL_CLAMP );

      float* max = new float[1];
      glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, max );

      glTexParameterf( GL_TEXTURE_2D, 
                     GL_TEXTURE_MAX_ANISOTROPY_EXT, 
                     max[0] );

      //for(i=0;i<width*height*3;i++)
        //cout << (int)(data[i]) << " ";
      //cout << endl;

      // build our texture mipmaps
      gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width, height,
                       GL_RGB, GL_FLOAT, data );

      delete [] max;

    }


    void Draw();
    void DrawTextured();
    void DrawLines();
   



    friend ostream &operator<<(ostream& stream,Face& obj){
      stream << " [ " << " face( " << obj.name << " ) : ";
//      stream << " halfedge = " << obj.halfedge->name << " ; ";
      stream << " vertex = { ";
      for(int i=0;i<obj.v.size();i++)
        stream << obj.v[i]->name << " ; ";
      stream << " } ";
      stream << " ] ";
      return stream;
    }



 
};


class Edge{

public:

  string name;

  Vertex* _replace;

  Vertex* _remove;

  Edge* __e;

  Face* f0;

  Face* f1;

  vector<Face*> remove_faces;

  vector<Face*> dont_remove_faces;

  vector<Vertex*> V;

  vector<Vertex*> U;

  vector<Vertex*>::iterator it;

  Vector center;

  int i;

  int I;

  int size;





  double weight;

  bool is_valid;

  HalfEdge* halfedge;

  pair<Vertex*,Vertex*> v;

  Edge(){

    halfedge = NULL;

    is_valid = true;

    weight = 0;


  }

  Edge(Vertex* v1,Vertex* v2){

    halfedge = NULL;

    v.first = v1;

    v.second = v2;

    is_valid = true;


    weight = 0;

  }



  friend ostream &operator<<(ostream& stream,Edge& obj);



  vector<Edge*> temp;


  bool valid(){






  vector<Vertex*>::iterator it;

  int i;

  int size;

    Vertex* _remove = v.first;
    Vertex* _replace = v.second;

    V.clear();

    U.clear();

    vector<Vertex*>::iterator it2;

    size = _remove->out.size();
    for(i=0;i<size;i++){
      it = find(U.begin(),U.end(),_remove->out[i]->v.second);
      it2 = find(V.begin(),V.end(),_remove->out[i]->v.second);
      if(it == U.end() && it2 == V.end())
        U.push_back(_remove->out[i]->v.second);
    }

    size = _replace->out.size();
    for(i=0;i<size;i++){
      it = find(U.begin(),U.end(),_replace->out[i]->v.second);
      it2 = find(V.begin(),V.end(),_replace->out[i]->v.second);
      if(it == U.end() && it2 == V.end())
        U.push_back(_replace->out[i]->v.second);
    }

    for(i=0;i<U.size();i++)
      if(U[i]->find(_replace) != NULL && U[i]->find(_remove) != NULL)
        V.push_back(U[i]);

    if(V.size() == 2){

      is_valid = true;

      return true;

    }

    is_valid = false;

    return false;

  }

  

    
    vector<Edge*> contact;

};

Edge* Vertex::find(Vertex* v){
  for(int i=0;i<out.size();i++)
    if(out[i]->v.second == v)
      return out[i];
  return NULL;
}




class FindPeripheralsSignal: public Signal{
    void execute(void* arg);
};


struct Block {
    Menu* menu;
    bool visible;
    unsigned int type;
    int x, y, z;
    float light;
    float artificial_light;
    float health;
    float armor;
    float power;
    float signal;
    float air;
    float heat;
    int color;
    string name;
    vector<int> faces;
    Block(){
        visible = false;
        type = 1;
        x = 1;
        y = 1;
        z = 1;
        air = 0.0;
        light = 1.0;
        artificial_light = 0.0;
        health = 100;
        armor = 5;
        power = 0;
        heat = 0;
        color = 0;
        menu = NULL;
    }

    Block(int _x,int _y,int _z,unsigned int _type){
        visible = false;
        type = _type;
        x = _x;
        y = _y;
        z = _z;
        air = 0.0;
        light = 1.0;
        artificial_light = 0.0;
        health = 100;
        armor = 5;
        power = 0;
        heat = 0;
        color = 0;
        menu = NULL;
        switch(type){
            case 1:
                armor = 2;
                health = 100;
                break;
            case 2:
                armor = 4;
                health = 100;
                break;
            case 3:
                armor = 6;
                health = 100;
                break;
            case 4:
                armor = 4;
                health = 100;
                break;
            case 5:
                armor = 5;
                health = 100;
                break;
            case 12:
            {
                menu = new Menu();
                menu->x_min =-.2;
                menu->x_max = .2;
                menu->y_min =-.2;
                menu->y_max = .2;
                menu->texture = 3;

                for(float x=-.10;x<=0;x+=.05) 
                for(float y=-.10;y<=0;y+=.05) 
                {
                MenuInput* menu_c = new MenuInput();
                menu_c->x_min =x-.023;
                menu_c->x_max =x+.023;
                menu_c->y_min =y-.023;
                menu_c->y_max =y+.023;
                menu_c->texture = 6;
                menu_c->item = NULL;
                menu->component.push_back(menu_c);
                menu->input.push_back(menu_c);
                }
                {
                Button* button = new Button();
                button->x_min = 0.05;
                button->x_max = 0.20;
                button->y_min = 0.05;
                button->y_max = 0.10;
                button->label = "build";
                button->texture = 7;
                button->parent = menu;
                menu->signal[button->label] = new BuildSignal();
                menu->component.push_back(button);
                }
                {
                MenuOutput* menu_o = new MenuOutput();
                menu_o->x_min =.1-.023;
                menu_o->x_max =.1+.023;
                menu_o->y_min =-.1-.023;
                menu_o->y_max =-.1+.023;
                menu_o->texture = 8;
                menu_o->item = NULL;
                menu->component.push_back(menu_o);
                menu->output.push_back(menu_o);
                }
                break;
            }
            case 15:
            {
                menu = new Menu();
                menu->x_min =-.2;
                menu->x_max = .2;
                menu->y_min =-.2;
                menu->y_max = .2;
                menu->texture = 3;

                {
                MenuInput* menu_c = new MenuInput();
                menu_c->x_min =-.1-.023;
                menu_c->x_max =-.1+.023;
                menu_c->y_min =-.1-.023;
                menu_c->y_max =-.1+.023;
                menu_c->texture = 6;
                menu_c->item = NULL;
                menu->component.push_back(menu_c);
                menu->input.push_back(menu_c);
                }
                {
                Button* button = new Button();
                button->x_min = 0.05;
                button->x_max = 0.20;
                button->y_min = 0.05;
                button->y_max = 0.10;
                button->label = "refine";
                button->texture = 7;
                button->parent = menu;
                menu->signal[button->label] = new RefineSignal();
                menu->component.push_back(button);
                }

                {
                MenuOutput* menu_o = new MenuOutput();
                menu_o->x_min =.1-.023;
                menu_o->x_max =.1+.023;
                menu_o->y_min =-.1-.023;
                menu_o->y_max =-.1+.023;
                menu_o->texture = 8;
                menu_o->item = NULL;
                menu->component.push_back(menu_o);
                menu->output.push_back(menu_o);
                }
                break;
            }
            case 16:
            {

                menu = new Menu();
                menu->x_min =-.2;
                menu->x_max = .2;
                menu->y_min =-.2;
                menu->y_max = .2;
                menu->texture = 3;

                {
                MenuStringInput* menu_c = new MenuStringInput();
                menu_c->x_min =-.1-.023;
                menu_c->x_max =-.1+.023;
                menu_c->y_min =-.1-.023;
                menu_c->y_max =-.1+.023;
                menu_c->texture = 6;
                menu->component.push_back(menu_c);
                }

                {
                MenuNumericStringInput* menu_c = new MenuNumericStringInput();
                menu_c->x_min =-.1-.023;
                menu_c->x_max =-.1+.023;
                menu_c->y_min =-.1-.023+.04;
                menu_c->y_max =-.1+.023+.04;
                menu_c->texture = 6;
                menu->component.push_back(menu_c);
                }


                break;

            }
            case 17:
            {

                menu = new Menu();
                menu->x_min =-.2;
                menu->x_max = .2;
                menu->y_min =-.2;
                menu->y_max = .2;
                menu->texture = 3;

                vector<char> char_map;
                char_map.push_back('W');
                char_map.push_back('S');
                char_map.push_back('A');
                char_map.push_back('D');
                char_map.push_back('Q');
                char_map.push_back('Z');

                vector<char> group_map;
                group_map.push_back('1');
                group_map.push_back('2');
                group_map.push_back('3');
                group_map.push_back('4');
                group_map.push_back('5');
                group_map.push_back('6');

                menu->block_parent = this;

                float y = .24;
                for(int i=0;i<6;i++,y-=.06)
                {
                    {
                    MenuStringOutput* menu_c = new MenuStringOutput();
                    menu_c->x_min =-.1-.023-.06;
                    menu_c->x_max =-.1+.023-.06;
                    menu_c->y_min =-.1-.023+y;
                    menu_c->y_max =-.1+.023+y;
                    menu_c->texture = 6;
                    menu_c->name = char_map[i];
                    menu->component.push_back(menu_c);
                    }

                    {
                    MenuNumericStringInput* menu_c = new MenuNumericStringInput();
                    menu_c->x_min =-.1-.023;
                    menu_c->x_max =-.1+.023;
                    menu_c->y_min =-.1-.023+y;
                    menu_c->y_max =-.1+.023+y;
                    menu_c->texture = 6;
                    menu_c->name = group_map[i];
                    menu->component.push_back(menu_c);
                    }

                    {
                    MenuStringOutput* menu_c = new MenuStringOutput();
                    menu_c->x_min =-.1-.023+.06;
                    menu_c->x_max =-.1+.023+.06;
                    menu_c->y_min =-.1-.023+y;
                    menu_c->y_max =-.1+.023+y;
                    menu_c->texture = 6;
                    menu->component.push_back(menu_c);
                    }
                }

                {
                Button* button = new Button();
                button->x_min = 0.05;
                button->x_max = 0.20;
                button->y_min = 0.05;
                button->y_max = 0.10; 
                button->label = "find"; 
                button->texture = 7;
                button->parent = menu;
                menu->signal[button->label] = new FindPeripheralsSignal();
                menu->component.push_back(button);
                } 

                {
                MenuStringOutput* menu_c = new MenuStringOutput();
                menu_c->x_min =-.1-.023-.09;
                menu_c->x_max =-.1+.023-.09;
                menu_c->y_min =-.1-.023-.09;
                menu_c->y_max =-.1+.023-.09;
                menu_c->texture = 6;
                menu_c->name = "";
                menu->component.push_back(menu_c);
                }

                {
                MenuStringOutput* menu_c = new MenuStringOutput();
                menu_c->x_min =-.1-.023+.03;
                menu_c->x_max =-.1+.023+.03;
                menu_c->y_min =-.1-.023-.09;
                menu_c->y_max =-.1+.023-.09;
                menu_c->texture = 6;
                menu_c->name = "";
                menu->component.push_back(menu_c);
                }

                {
                MenuStringOutput* menu_c = new MenuStringOutput();
                menu_c->x_min =-.1-.023+.15;
                menu_c->x_max =-.1+.023+.15;
                menu_c->y_min =-.1-.023-.09;
                menu_c->y_max =-.1+.023-.09;
                menu_c->texture = 6;
                menu_c->name = "";
                menu->component.push_back(menu_c);
                }

                break;

            }
            case 14:
            {
                menu = new Menu();
                menu->x_min =-.2;
                menu->x_max = .2;
                menu->y_min =-.2;
                menu->y_max = .2;
                menu->texture = 3;

                for(float x=-.15;x<=.15;x+=.05) 
                for(float y=-.15;y<=.15;y+=.05) 
                {
                MenuInput* menu_c = new MenuInput();
                menu_c->x_min =x-.023;
                menu_c->x_max =x+.023;
                menu_c->y_min =y-.023;
                menu_c->y_max =y+.023;
                menu_c->texture = 6;
                menu_c->item = NULL;
                menu->component.push_back(menu_c);
                menu->input.push_back(menu_c);
                }
                break;
            }
            case 26:
            {
                menu = new Menu();
                menu->x_min =-.2;
                menu->x_max = .2;
                menu->y_min =-.2;
                menu->y_max = .2;
                menu->texture = 3;

                {
                MenuOutput* menu_c = new MenuOutput();
                menu_c->x_min =-.023;
                menu_c->x_max =+.023;
                menu_c->y_min =-.023;
                menu_c->y_max =+.023;
                menu_c->texture = 6;
                menu_c->item = NULL;
                menu->component.push_back(menu_c);
                menu->output.push_back(menu_c);
                }
                break;
            }
            case 27:
            {
                menu = new Menu();
                menu->x_min =-.2;
                menu->x_max = .2;
                menu->y_min =-.2;
                menu->y_max = .2;
                menu->texture = 3;

                {
                MenuInput* menu_c = new MenuInput();
                menu_c->x_min =-.023;
                menu_c->x_max =+.023;
                menu_c->y_min =-.03-.023;
                menu_c->y_max =-.03+.023;
                menu_c->texture = 6;
                menu_c->item = NULL;
                menu->component.push_back(menu_c);
                menu->input.push_back(menu_c);
                }

                {
                MenuOutput* menu_c = new MenuOutput();
                menu_c->x_min =-.023;
                menu_c->x_max =+.023;
                menu_c->y_min =+.03-.023;
                menu_c->y_max =+.03+.023;
                menu_c->texture = 6;
                menu_c->item = NULL;
                menu->component.push_back(menu_c);
                menu->output.push_back(menu_c);
                }
                break;
            }
            case 30:
            {
                menu = new Menu();
                menu->x_min =-.2;
                menu->x_max = .2;
                menu->y_min =-.2;
                menu->y_max = .2;
                menu->texture = 3;

                {
                MenuInput* menu_c = new MenuInput();
                menu_c->x_min =-.023;
                menu_c->x_max =+.023;
                menu_c->y_min =-.03-.023;
                menu_c->y_max =-.03+.023;
                menu_c->texture = 6;
                menu_c->item = NULL;
                menu->component.push_back(menu_c);
                menu->input.push_back(menu_c);
                }

                break;
            }
        }
    }

};

Block* selection = NULL;

#define imageWidth  4096
#define imageHeight 4096

#define JimageWidth  1024
#define JimageHeight 1024

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

class JupiterCloudGenerator
{

public:

    GLubyte checkImage[JimageHeight][JimageWidth][4];
    GLubyte ret_checkImage[JimageHeight][JimageWidth][4];

    GLuint texName;

    FractalNoise * noiseMaker;

    JupiterCloudGenerator()
    {
        noiseMaker = new FractalNoise();
        noiseMaker->setBaseFrequency(4.0f);
    }

    void makePerlinImage(float alpha)
    {

        float z = alpha;

        float noise;

        float cloud_noise;

        float temp;

        signed char colourByte;

        float min_val = -1.5f;
        float max_val = 1.5f;
        float r_fact = 1;
        float b_fact = 1;
        float g_fact = 1;

	    for (int x=0; x<JimageWidth; ++x) for (int y=0; y<JimageHeight; ++y) {
	        /*
	    	noise = noiseMaker->noise( float(20.0*(z*.001+1+cos(2*3.14159*(x+z*.075)/JimageWidth)*sin(3.14159*(y)/JimageHeight)))
                                     , float(20.0*(z*.001+sin(z*.001)+cos(3.14159*(y)/JimageHeight)))
                                     , float(20.0*(z*.001+sin(2*3.14159*(x+z*.075)/JimageWidth)*sin(3.14159*(y)/JimageHeight)))
                                     );
            */
	    	noise = noiseMaker->noise( float(2.0*(z*.001+1+cos(2*3.14159*(x+z*.075)/JimageWidth)*sin(3.14159*(y)/JimageHeight)))
                                     , float(2.0*(z*.001+sin(z*.001)+cos(3.14159*(y)/JimageHeight)))
                                     , float(2.0*(z*.001+sin(2*3.14159*(x+z*.075)/JimageWidth)*sin(3.14159*(y)/JimageHeight)))
                                     );

            g_fact = noise*noise*2.5;
            if(g_fact>1.0)
                g_fact=1.0;
            r_fact = g_fact;
            b_fact = 1.0 - r_fact;

            temp = 1.0/(max_val-min_val);
	    	noise = -1.0f + 2.0f*(noise - min_val)*temp;
	    	noise += 1.0f;
	    	noise *= 0.5f;

	    	colourByte = (signed char)((std::max(255.0,(noise)*5.0))*0xff);
            //if(noise < .43)
            //{
	    	//    checkImage[y][x][0] = (GLubyte)0;
	    	//    checkImage[y][x][1] = (GLubyte)0;
	    	//    checkImage[y][x][2] = (GLubyte)0;
	    	//    checkImage[y][x][3] = (GLubyte)0;
            //}
            //else
            {
	    	    checkImage[y][x][0] = (GLubyte)(colourByte*r_fact);
	    	    checkImage[y][x][1] = (GLubyte)(colourByte*g_fact);
	    	    checkImage[y][x][2] = (GLubyte)(colourByte*b_fact);
	    	    checkImage[y][x][3] = (GLubyte)colourByte;
            }

	    }


    }

    void makeImage()
    {

        int i, j, c;
         
        for (i = 0; i < JimageHeight; i++) {
           for (j = 0; j < JimageWidth; j++) {
              c = ((((i&0x8)==0)^((j&0x8))==0))*255;
              checkImage[i][j][0] = (GLubyte) c;
              checkImage[i][j][1] = (GLubyte) c*.15;
              checkImage[i][j][2] = (GLubyte) c*.15;
              checkImage[i][j][3] = (GLubyte) 255;
           }
        }

    }

    void generateTexture(float alpha)
    {

        makePerlinImage(alpha);

    }

};

JupiterCloudGenerator Jupiter_cloud_generator;

void * CalculateJupiterTexture(void * params)
{
    JupiterCloudGenerator* generator = (JupiterCloudGenerator*)params;
    float z = 100;
    while(1)
    {
        Jupiter_cloud_generator.generateTexture(z);
        pthread_mutex_lock(&lock);
        memcpy(Jupiter_cloud_generator.ret_checkImage,Jupiter_cloud_generator.checkImage,sizeof (GLubyte) * JimageHeight * JimageWidth * 4);
        signal_Jupiter_texture = true;
        pthread_mutex_unlock(&lock);
        z += 0.01;
        //break;
        usleep(100000);
        //usleep(1000);
    }
    pthread_exit(NULL);
}

class CloudGenerator
{

public:

    GLubyte checkImage[imageHeight][imageWidth][4];

    GLuint texName;

    void makePerlinImage()
    {
        FractalNoise * noiseMaker = new FractalNoise();
        noiseMaker->setBaseFrequency(4.0f);

        float z = 100.72;

        float noise;

        float cloud_noise;

        float temp;

        signed char colourByte;

        float min_val = -1.5f;
        float max_val = 1.5f;
        float r_fact = 1;
        float b_fact = 1;
        float g_fact = 1;

	    for (int x=0; x<imageWidth; ++x) for (int y=0; y<imageHeight; ++y) {
	    
	    	noise = noiseMaker->noise( float(1000005.3*(z*.001+1+cos(2*3.14159*(x+z*.075)/imageWidth)*sin(3.14159*(y)/imageHeight)))
                                     , float(1000005.3*(z*.001+sin(z*.001)+cos(3.14159*(y)/imageHeight)))
                                     , float(1000005.3*(z*.001+sin(2*3.14159*(x+z*.075)/imageWidth)*sin(3.14159*(y)/imageHeight)))
                                     );
 
	    	cloud_noise = noiseMaker->noise( float(1.0*(z*.001+1+cos(2*3.14159*(x+z*.075)/imageWidth)*sin(3.14159*(y)/imageHeight)))
                                           , float(1.0*(z*.001+sin(z*.001)+cos(3.14159*(y)/imageHeight)))
                                           , float(1.0*(z*.001+sin(2*3.14159*(x+z*.075)/imageWidth)*sin(3.14159*(y)/imageHeight)))
                                           );

            r_fact = 1;
            g_fact = 1;
            b_fact = 1;

            if(cloud_noise > noise)
            {
                noise = cloud_noise;
                b_fact = noise*noise*2.5;
                if(b_fact>1.0)
                    b_fact=1.0;
                r_fact = 1.0 - b_fact;
            }

            temp = 1.0/(max_val-min_val);
	    	noise = -1.0f + 2.0f*(noise - min_val)*temp;
	    	noise += 1.0f;
	    	noise *= 0.5f;

	    	colourByte = (signed char)(((noise-.53)*0.5)*0xff);
            if(noise < .53)
            {
	    	    checkImage[y][x][0] = (GLubyte)0;
	    	    checkImage[y][x][1] = (GLubyte)0;
	    	    checkImage[y][x][2] = (GLubyte)0;
	    	    checkImage[y][x][3] = (GLubyte)0;
            }
            else
            {
	    	    checkImage[y][x][0] = (GLubyte)(colourByte*r_fact);
	    	    checkImage[y][x][1] = (GLubyte)(colourByte*g_fact);
	    	    checkImage[y][x][2] = (GLubyte)(colourByte*b_fact);
	    	    checkImage[y][x][3] = (GLubyte)colourByte;
            }

	    }


    }

    void makeImage()
    {

        int i, j, c;
         
        for (i = 0; i < imageHeight; i++) {
           for (j = 0; j < imageWidth; j++) {
              c = ((((i&0x8)==0)^((j&0x8))==0))*255;
              checkImage[i][j][0] = (GLubyte) c;
              checkImage[i][j][1] = (GLubyte) c*.15;
              checkImage[i][j][2] = (GLubyte) c*.15;
              checkImage[i][j][3] = (GLubyte) 255;
           }
        }

    }

    void generateTexture()
    {

        makePerlinImage();
        glPixelStorei( GL_UNPACK_ALIGNMENT
                     , 1
                     );

        glGenTextures( 1
                     , &texName
                     );
        glBindTexture( GL_TEXTURE_2D
                     , texName
                     );

        glTexParameteri( GL_TEXTURE_2D
                       , GL_TEXTURE_WRAP_S
                       , GL_REPEAT
                       );
        glTexParameteri( GL_TEXTURE_2D
                       , GL_TEXTURE_WRAP_T
                       , GL_REPEAT
                       );
        glTexParameteri( GL_TEXTURE_2D
                       , GL_TEXTURE_MAG_FILTER
                       , GL_NEAREST
                       );
        glTexParameteri( GL_TEXTURE_2D
                       , GL_TEXTURE_MIN_FILTER
                       , GL_NEAREST
                       );
        glTexImage2D( GL_TEXTURE_2D
                    , 0
                    , GL_RGBA
                    , imageWidth
                    , imageHeight
                    , 0
                    , GL_RGBA
                    , GL_UNSIGNED_BYTE
                    , checkImage
                    );

    }

};

typedef struct					// Create A Structure For Particle
{
	bool	active;				// Active (Yes/No)
	float	life;				// Particle Life
	float	fade;				// Fade Speed
	float	r;					// Red Value
	float	g;					// Green Value
	float	b;					// Blue Value
	float	x;					// X Position
	float	y;					// Y Position
	float	z;					// Z Position
	float	xi;					// X Direction
	float	yi;					// Y Direction
	float	zi;					// Z Direction
	float	xg;					// X Gravity
	float	yg;					// Y Gravity
	float	zg;					// Z Gravity
    float   width;              // width of textured plane
}
particles;						// Particles Structure

class Mesh;

class Mesh{

  public:

    vector<Face*> face;
    vector<Vertex*> vertex;
    vector<Edge*> edge;
    vector<HalfEdge*> v_halfedge;

    int m_texture;

    void read(string name,float scale = 1.0){
      cout << "opening file: " << name.c_str() << endl;
      fstream file;
      file.open(name.c_str());
      if(file.is_open()){
        vertex.push_back(NULL); // indexing starts at 1 
        cout << "file open" << endl;
        cout << "reading file, please wait ..." << endl;
        string s;
        string input;
        Vector center;
        while(file.good()){
          getline(file,s);
          stringstream ss(s);
          ss >> input;
          if(strstr(input.c_str(),"v")){
            ss >> input;
            double x = atof(input.c_str())*scale;
            ss >> input;
            double y = atof(input.c_str())*scale;
            ss >> input;
            double z = atof(input.c_str())*scale;
            vertex.push_back(new Vertex(x,y,z));
            center.p[0] += x;
            center.p[1] += y;
            center.p[2] += z;
          }
          if(strstr(input.c_str(),"f")){
            Face* f = new Face();
            int index;
            while(ss >> input){
              index = atoi(input.c_str());
              f->v.push_back(vertex[index]);
            }
            f->size = f->v.size();
            int p,n;
            for(int i=0;i<f->size;i++){
              n = (i+1)%f->size;
              p = (i+f->size-1)%f->size;
              if(f->v[i]->find(f->v[n]) == NULL){
                Edge* e = new Edge(f->v[i],f->v[n]);
                f->v[i]->out.push_back(e);
                edge.push_back(e);
              }
              if(f->v[i]->find(f->v[p]) == NULL){
                Edge* e = new Edge(f->v[i],f->v[p]);
                f->v[i]->out.push_back(e);
                edge.push_back(e);
              }
            }
            f->setnormal();
            for(int i=0;i<f->size;i++){
              HalfEdge* he = new HalfEdge();
              v_halfedge.push_back(he);
              he->face = f;
              f->halfedge = he;
              Edge* e_a = f->v[i]->find(f->v[(i+1)%f->size]);
              Edge* e_b = f->v[(i+1)%f->size]->find(f->v[i]);
              he->edge_a = e_a;
              he->edge_b = e_b;
              if(e_a->halfedge == NULL || e_b->halfedge == NULL){
                e_a->halfedge = he;
                e_b->halfedge = he;
              }else{
                he->flip = e_a->halfedge;
                e_a->halfedge->flip = he;
              }
              he->origin = f->v[i];
              f->v[i]->halfedge = he;
            }
            f->color_w = 1.0;
            f->color_r = 1.0;
            f->color_g = 1.0;
            f->color_b = 1.0;
            f->texture = 0;
            face.push_back(f);
          }
        }
        int size = (int)vertex.size()-1;
        center.p[0] /= size;
        center.p[1] /= size;
        center.p[2] /= size;
//        for(int i=1;i<vertex.size();i++){
//          vertex[i]->p[0] -= center.p[0];
//          vertex[i]->p[1] -= center.p[1];
//          vertex[i]->p[2] -= center.p[2];
//        }
        double average_size = 0;
        for(int i=1;i<vertex.size();i++){
          average_size += sqrt(vertex[i]->p[0]*vertex[i]->p[0]+vertex[i]->p[1]*vertex[i]->p[1]+vertex[i]->p[2]*vertex[i]->p[2]);
        }
        average_size /= (int)vertex.size() - 1;
        double scale = 5/average_size;
//        for(int i=1;i<vertex.size();i++){
//          vertex[i]->p[0] *= scale;
//          vertex[i]->p[1] *= scale;
//          vertex[i]->p[2] *= scale;
//        }
        file.close();
        cout << "file closed" << endl;
      }else
        cout << "failed to open file" << endl;

      cout << vertex.size() << " " << edge.size()/2.0 << " " << face.size() << " " << v_halfedge.size() << endl;


      cout << "verify Euler characteristic: V - E + F = "
        << (int)(vertex.size()-1) << "-" << (int)edge.size()/2 << "+" << (int)face.size() 
        << " = 2*(1+G) = " << (int)(vertex.size()-1) - (int)edge.size()/2 + (int)face.size() << endl;


    }


    void RemoveBox( Block* block) {

        //std::cout << "size=" << block->faces.size() << std::endl;

        for(int i=0;i<block->faces.size();i++){
            //std::cout << "ind=" << block->faces[i] << std::endl;
            face[block->faces[i]]->v[0] = new Vertex(0,0,0);
            face[block->faces[i]]->v[1] = new Vertex(0,0,0);
            face[block->faces[i]]->v[2] = new Vertex(0,0,0);
            //face.erase(face.begin()+block->faces[i]);
        }

        //block->faces.clear();

    }

    void AddBox( double x
               , double y
               , double z
               , double wx
               , double wy
               , double wz
               , int p_texture
               , Block* block = NULL
               , double opaque = 1.0
               , bool rounded = false

               , bool x_positive = false
               , bool x_negative = false
               , bool y_positive = false
               , bool y_negative = false
               , bool z_positive = false
               , bool z_negative = false

               , bool ppp_corner = false
               , bool ppm_corner = false
               , bool pmp_corner = false
               , bool pmm_corner = false
               , bool mpp_corner = false
               , bool mpm_corner = false
               , bool mmp_corner = false
               , bool mmm_corner = false

               , bool ppz_corner = false
               , bool pmz_corner = false
               , bool mpz_corner = false
               , bool mmz_corner = false
               , bool pzp_corner = false
               , bool pzm_corner = false
               , bool mzp_corner = false
               , bool mzm_corner = false
               , bool zpp_corner = false
               , bool zpm_corner = false
               , bool zmp_corner = false
               , bool zmm_corner = false

               )
    {
/*  

            glTexCoord2d(0.0,0.0); glVertex3f( scale, scale,-scale);    // Top Right Of The Quad (Top)
            glTexCoord2d(1.0,0.0); glVertex3f(-scale, scale,-scale);    // Top Left Of The Quad (Top)
            glTexCoord2d(1.0,1.0); glVertex3f(-scale, scale, scale);    // Bottom Left Of The Quad (Top)
            glTexCoord2d(0.0,1.0); glVertex3f( scale, scale, scale);    // Bottom Right Of The Quad (Top)

            glTexCoord2d(0.0,0.0); glVertex3f( scale,-scale, scale);    // Top Right Of The Quad (Bottom)
            glTexCoord2d(1.0,0.0); glVertex3f(-scale,-scale, scale);    // Top Left Of The Quad (Bottom)
            glTexCoord2d(1.0,1.0); glVertex3f(-scale,-scale,-scale);    // Bottom Left Of The Quad (Bottom)
            glTexCoord2d(0.0,1.0); glVertex3f( scale,-scale,-scale);    // Bottom Right Of The Quad (Bottom)

            glTexCoord2d(0.0,0.0); glVertex3f( scale, scale, scale);    // Top Right Of The Quad (Front)
            glTexCoord2d(1.0,0.0); glVertex3f(-scale, scale, scale);    // Top Left Of The Quad (Front)
            glTexCoord2d(1.0,1.0); glVertex3f(-scale,-scale, scale);    // Bottom Left Of The Quad (Front)
            glTexCoord2d(0.0,1.0); glVertex3f( scale,-scale, scale);    // Bottom Right Of The Quad (Front)

            glTexCoord2d(0.0,0.0); glVertex3f( scale,-scale,-scale);    // Top Right Of The Quad (Back)
            glTexCoord2d(1.0,0.0); glVertex3f(-scale,-scale,-scale);    // Top Left Of The Quad (Back)
            glTexCoord2d(1.0,1.0); glVertex3f(-scale, scale,-scale);    // Bottom Left Of The Quad (Back)
            glTexCoord2d(0.0,1.0); glVertex3f( scale, scale,-scale);    // Bottom Right Of The Quad (Back)

            glTexCoord2d(0.0,0.0); glVertex3f(-scale, scale, scale);    // Top Right Of The Quad (Left)
            glTexCoord2d(1.0,0.0); glVertex3f(-scale, scale,-scale);    // Top Left Of The Quad (Left)
            glTexCoord2d(1.0,1.0); glVertex3f(-scale,-scale,-scale);    // Bottom Left Of The Quad (Left)
            glTexCoord2d(0.0,1.0); glVertex3f(-scale,-scale, scale);    // Bottom Right Of The Quad (Left)

            glTexCoord2d(0.0,0.0); glVertex3f( scale, scale,-scale);    // Top Right Of The Quad (Right)
            glTexCoord2d(1.0,0.0); glVertex3f( scale, scale, scale);    // Top Left Of The Quad (Right)
            glTexCoord2d(1.0,1.0); glVertex3f( scale,-scale, scale);    // Bottom Left Of The Quad (Right)
            glTexCoord2d(0.0,1.0); glVertex3f( scale,-scale,-scale);    // Bottom Right Of The Quad (Right)

*/

    if(p_texture == 28)
        opaque = 0.5;

    if(block)
    {
        block->visible = true;
        block->faces.clear();
    }

    m_texture = p_texture;

    Vertex* vppp = new Vertex(x+wx,y+wy,z+wz);
    Vertex* vmpp = new Vertex(x-wx,y+wy,z+wz);
    Vertex* vpmp = new Vertex(x+wx,y-wy,z+wz);
    Vertex* vmmp = new Vertex(x-wx,y-wy,z+wz);
    Vertex* vppm = new Vertex(x+wx,y+wy,z-wz);
    Vertex* vmpm = new Vertex(x-wx,y+wy,z-wz);
    Vertex* vpmm = new Vertex(x+wx,y-wy,z-wz);
    Vertex* vmmm = new Vertex(x-wx,y-wy,z-wz);

    if( rounded )
    {
        if( x_positive == false && x_negative == true && y_positive == false && y_negative == true && z_positive == true && z_negative == true )
        {
            vppp->p[0] = (vmpp->p[0]);
            vppp->p[1] = (vmpp->p[1]);
            vppp->p[2] = (vmpp->p[2]);

            vppm->p[0] = (vmpm->p[0]);
            vppm->p[1] = (vmpm->p[1]);
            vppm->p[2] = (vmpm->p[2]);

        } else
        if( x_positive == true && x_negative == false && y_positive == false && y_negative == true && z_positive == true && z_negative == true )
        {
            vmpp->p[0] = (vmmp->p[0]);
            vmpp->p[1] = (vmmp->p[1]);
            vmpp->p[2] = (vmmp->p[2]);

            vmpm->p[0] = (vmmm->p[0]);
            vmpm->p[1] = (vmmm->p[1]);
            vmpm->p[2] = (vmmm->p[2]);

        } else
        if( x_positive == false && x_negative == true && y_positive == true && y_negative == false && z_positive == true && z_negative == true )
        {
            vpmp->p[0] = (vppp->p[0]);
            vpmp->p[1] = (vppp->p[1]);
            vpmp->p[2] = (vppp->p[2]);

            vpmm->p[0] = (vppm->p[0]);
            vpmm->p[1] = (vppm->p[1]);
            vpmm->p[2] = (vppm->p[2]);

        } else
        if( x_positive == true && x_negative == false && y_positive == true && y_negative == false && z_positive == true && z_negative == true )
        {
            vmmp->p[0] = (vpmp->p[0]);
            vmmp->p[1] = (vpmp->p[1]);
            vmmp->p[2] = (vpmp->p[2]);

            vmmm->p[0] = (vpmm->p[0]);
            vmmm->p[1] = (vpmm->p[1]);
            vmmm->p[2] = (vpmm->p[2]);

        } else
        if( x_positive == false && x_negative == true && z_positive == false && z_negative == true && y_positive == true && y_negative == true )
        {
            vppp->p[0] = (vmpp->p[0]);
            vppp->p[1] = (vmpp->p[1]);
            vppp->p[2] = (vmpp->p[2]);

            vpmp->p[0] = (vmmp->p[0]);
            vpmp->p[1] = (vmmp->p[1]);
            vpmp->p[2] = (vmmp->p[2]);

        } else
        if( x_positive == true && x_negative == false && z_positive == false && z_negative == true && y_positive == true && y_negative == true )
        {
            vmpp->p[0] = (vmpm->p[0]);
            vmpp->p[1] = (vmpm->p[1]);
            vmpp->p[2] = (vmpm->p[2]);

            vmmp->p[0] = (vmmm->p[0]);
            vmmp->p[1] = (vmmm->p[1]);
            vmmp->p[2] = (vmmm->p[2]);

        } else
        if( x_positive == false && x_negative == true && z_positive == true && z_negative == false && y_positive == true && y_negative == true )
        {
            vppm->p[0] = (vppp->p[0]);
            vppm->p[1] = (vppp->p[1]);
            vppm->p[2] = (vppp->p[2]);

            vpmm->p[0] = (vpmp->p[0]);
            vpmm->p[1] = (vpmp->p[1]);
            vpmm->p[2] = (vpmp->p[2]);

        } else
        if( x_positive == true && x_negative == false && z_positive == true && z_negative == false && y_positive == true && y_negative == true )
        {
            vmpm->p[0] = (vppm->p[0]);
            vmpm->p[1] = (vppm->p[1]);
            vmpm->p[2] = (vppm->p[2]);

            vmmm->p[0] = (vpmm->p[0]);
            vmmm->p[1] = (vpmm->p[1]);
            vmmm->p[2] = (vpmm->p[2]);

        } else
        if( z_positive == false && z_negative == true && y_positive == false && y_negative == true && x_positive == true && x_negative == true )
        {
            vppp->p[0] = (vppm->p[0]);
            vppp->p[1] = (vppm->p[1]);
            vppp->p[2] = (vppm->p[2]);

            vmpp->p[0] = (vmpm->p[0]);
            vmpp->p[1] = (vmpm->p[1]);
            vmpp->p[2] = (vmpm->p[2]);

        } else
        if( z_positive == true && z_negative == false && y_positive == false && y_negative == true && x_positive == true && x_negative == true )
        {
            vppm->p[0] = (vpmm->p[0]);
            vppm->p[1] = (vpmm->p[1]);
            vppm->p[2] = (vpmm->p[2]);

            vmpm->p[0] = (vmmm->p[0]);
            vmpm->p[1] = (vmmm->p[1]);
            vmpm->p[2] = (vmmm->p[2]);

        } else
        if( z_positive == false && z_negative == true && y_positive == true && y_negative == false && x_positive == true && x_negative == true )
        {
            vpmp->p[0] = (vppp->p[0]);
            vpmp->p[1] = (vppp->p[1]);
            vpmp->p[2] = (vppp->p[2]);

            vmmp->p[0] = (vmpp->p[0]);
            vmmp->p[1] = (vmpp->p[1]);
            vmmp->p[2] = (vmpp->p[2]);

        } else
        if( z_positive == true && z_negative == false && y_positive == true && y_negative == false && x_positive == true && x_negative == true )
        {
            vpmm->p[0] = (vpmp->p[0]);
            vpmm->p[1] = (vpmp->p[1]);
            vpmm->p[2] = (vpmp->p[2]);

            vmmm->p[0] = (vmmp->p[0]);
            vmmm->p[1] = (vmmp->p[1]);
            vmmm->p[2] = (vmmp->p[2]);

        } else
        if( x_positive == false && x_negative == true && y_positive == false && y_negative == true && z_positive == false && z_negative == true )
        {
            vppm->p[0] = (vmpm->p[0]);
            vppm->p[1] = (vmpm->p[1]);
            vppm->p[2] = (vmpm->p[2]);

            vmpp->p[0] = (vmmp->p[0]);
            vmpp->p[1] = (vmmp->p[1]);
            vmpp->p[2] = (vmmp->p[2]);

            vpmp->p[0] = (vpmm->p[0]);
            vpmp->p[1] = (vpmm->p[1]);
            vpmp->p[2] = (vpmm->p[2]);

            vppp->p[0] = (vpmm->p[0]);
            vppp->p[1] = (vpmm->p[1]);
            vppp->p[2] = (vpmm->p[2]);

        } else
        if( x_negative == true && y_negative == true && z_negative == true && ppp_corner == false && ppz_corner == false && pzp_corner == false && zpp_corner == false )
        {
            vppp->p[0] = (vppm->p[0]);
            vppp->p[1] = (vppm->p[1]);
            vppp->p[2] = (vppm->p[2]);

        } else
        if( x_positive == true && x_negative == false && y_positive == false && y_negative == true && z_positive == false && z_negative == true )
        {
            vmmp->p[0] = (vpmp->p[0]);
            vmmp->p[1] = (vpmp->p[1]);
            vmmp->p[2] = (vpmp->p[2]);

            vppp->p[0] = (vppm->p[0]);
            vppp->p[1] = (vppm->p[1]);
            vppp->p[2] = (vppm->p[2]);

            vmpm->p[0] = (vmmm->p[0]);
            vmpm->p[1] = (vmmm->p[1]);
            vmpm->p[2] = (vmmm->p[2]);

            vmpp->p[0] = (vmmm->p[0]);
            vmpp->p[1] = (vmmm->p[1]);
            vmpp->p[2] = (vmmm->p[2]);

        } else
        if( x_positive == true && y_negative == true && z_negative == true && mpp_corner == false && mpz_corner == false && mzp_corner == false && zpp_corner == false )
        {
            vmpp->p[0] = (vppp->p[0]);
            vmpp->p[1] = (vppp->p[1]);
            vmpp->p[2] = (vppp->p[2]);

        } else
        if( x_positive == false && x_negative == true && y_positive == true && y_negative == false && z_positive == false && z_negative == true )
        {
            vppp->p[0] = (vppm->p[0]);
            vppp->p[1] = (vppm->p[1]);
            vppp->p[2] = (vppm->p[2]);

            vpmm->p[0] = (vmmm->p[0]);
            vpmm->p[1] = (vmmm->p[1]);
            vpmm->p[2] = (vmmm->p[2]);

            vmmp->p[0] = (vmpp->p[0]);
            vmmp->p[1] = (vmpp->p[1]);
            vmmp->p[2] = (vmpp->p[2]);

            vpmp->p[0] = (vmpp->p[0]);
            vpmp->p[1] = (vmpp->p[1]);
            vpmp->p[2] = (vmpp->p[2]);

        } else
        if( x_negative == true && y_positive == true && z_negative == true && pmp_corner == false && pmz_corner == false && pzp_corner == false && zmp_corner == false )
        {
            vpmp->p[0] = (vppp->p[0]);
            vpmp->p[1] = (vppp->p[1]);
            vpmp->p[2] = (vppp->p[2]);

        } else
        if( x_positive == true && x_negative == false && y_positive == true && y_negative == false && z_positive == false && z_negative == true )
        {
            vpmp->p[0] = (vpmm->p[0]);
            vpmp->p[1] = (vpmm->p[1]);
            vpmp->p[2] = (vpmm->p[2]);

            vmmm->p[0] = (vmpm->p[0]);
            vmmm->p[1] = (vmpm->p[1]);
            vmmm->p[2] = (vmpm->p[2]);

            vmpp->p[0] = (vppp->p[0]);
            vmpp->p[1] = (vppp->p[1]);
            vmpp->p[2] = (vppp->p[2]);

            vmmp->p[0] = (vppp->p[0]);
            vmmp->p[1] = (vppp->p[1]);
            vmmp->p[2] = (vppp->p[2]);

        } else
        if( x_positive == true && y_positive == true && z_negative == true && mmp_corner == false && mmz_corner == false && mzp_corner == false && zmp_corner == false )
        {
            vmmp->p[0] = (vmmm->p[0]);
            vmmp->p[1] = (vmmm->p[1]);
            vmmp->p[2] = (vmmm->p[2]);

        } else
        if( x_positive == true && x_negative == false && y_positive == true && y_negative == false && z_positive == true && z_negative == false )
        {
            vpmm->p[0] = (vppm->p[0]);
            vpmm->p[1] = (vppm->p[1]);
            vpmm->p[2] = (vppm->p[2]);

            vmpm->p[0] = (vmpp->p[0]);
            vmpm->p[1] = (vmpp->p[1]);
            vmpm->p[2] = (vmpp->p[2]);

            vmmp->p[0] = (vpmp->p[0]);
            vmmp->p[1] = (vpmp->p[1]);
            vmmp->p[2] = (vpmp->p[2]);

            vmmm->p[0] = (vpmp->p[0]);
            vmmm->p[1] = (vpmp->p[1]);
            vmmm->p[2] = (vpmp->p[2]);

        } else
        if( x_positive == true && y_positive == true && z_positive == true && mmm_corner == false && mmz_corner == false && mzm_corner == false && zmm_corner == false )
        {
            vmmm->p[0] = (vmmp->p[0]);
            vmmm->p[1] = (vmmp->p[1]);
            vmmm->p[2] = (vmmp->p[2]);

        } else
        if( x_positive == false && x_negative == true && y_positive == true && y_negative == false && z_positive == true && z_negative == false )
        {
            vppm->p[0] = (vmpm->p[0]);
            vppm->p[1] = (vmpm->p[1]);
            vppm->p[2] = (vmpm->p[2]);

            vmmm->p[0] = (vmmp->p[0]);
            vmmm->p[1] = (vmmp->p[1]);
            vmmm->p[2] = (vmmp->p[2]);

            vpmp->p[0] = (vppp->p[0]);
            vpmp->p[1] = (vppp->p[1]);
            vpmp->p[2] = (vppp->p[2]);

            vpmm->p[0] = (vppp->p[0]);
            vpmm->p[1] = (vppp->p[1]);
            vpmm->p[2] = (vppp->p[2]);

        } else
        if( x_negative == true && y_positive == true && z_positive == true && pmm_corner == false && pmz_corner == false && pzm_corner == false && zmm_corner == false )
        {
            vpmm->p[0] = (vmmm->p[0]);
            vpmm->p[1] = (vmmm->p[1]);
            vpmm->p[2] = (vmmm->p[2]);

        } else
        if( x_positive == true && x_negative == false && y_positive == false && y_negative == true && z_positive == true && z_negative == false )
        {
            vppm->p[0] = (vpmm->p[0]);
            vppm->p[1] = (vpmm->p[1]);
            vppm->p[2] = (vpmm->p[2]);

            vmmm->p[0] = (vmmp->p[0]);
            vmmm->p[1] = (vmmp->p[1]);
            vmmm->p[2] = (vmmp->p[2]);

            vmpp->p[0] = (vppp->p[0]);
            vmpp->p[1] = (vppp->p[1]);
            vmpp->p[2] = (vppp->p[2]);

            vmpm->p[0] = (vppp->p[0]);
            vmpm->p[1] = (vppp->p[1]);
            vmpm->p[2] = (vppp->p[2]);

        } else
        if( x_positive == true && y_negative == true && z_positive == true && mpm_corner == false && mpz_corner == false && mzm_corner == false && zpm_corner == false )
        {
            vmpm->p[0] = (vmmm->p[0]);
            vmpm->p[1] = (vmmm->p[1]);
            vmpm->p[2] = (vmmm->p[2]);

        } else
        if( x_positive == false && x_negative == true && y_positive == false && y_negative == true && z_positive == true && z_negative == false )
        {
            vpmm->p[0] = (vmmm->p[0]);
            vpmm->p[1] = (vmmm->p[1]);
            vpmm->p[2] = (vmmm->p[2]);

            vppp->p[0] = (vpmp->p[0]);
            vppp->p[1] = (vpmp->p[1]);
            vppp->p[2] = (vpmp->p[2]);

            vmpm->p[0] = (vppp->p[0]);
            vmpm->p[1] = (vppp->p[1]);
            vmpm->p[2] = (vppp->p[2]);

            vppm->p[0] = (vmpp->p[0]);
            vppm->p[1] = (vmpp->p[1]);
            vppm->p[2] = (vmpp->p[2]);

        } else
        if( x_negative == true && y_negative == true && z_positive == true && ppm_corner == false && ppz_corner == false && pzm_corner == false && zpm_corner == false )
        {
            vppm->p[0] = (vppp->p[0]);
            vppm->p[1] = (vppp->p[1]);
            vppm->p[2] = (vppp->p[2]);

        }












    }

    {
      Face* f = new Face();
      f->v.push_back(vppp);
      f->v.push_back(vmpp);
      f->v.push_back(vpmp);
      f->color_w = opaque; 
      f->setnormal();
      f->texture = m_texture;
      if(block)
      block->faces.push_back(face.size());
      face.push_back(f);
    }

    {
      Face* f = new Face();
      f->v.push_back(vmmp);
      f->v.push_back(vpmp);
      f->v.push_back(vmpp);
      f->color_w = opaque; 
      f->setnormal();
      f->texture = m_texture;
      if(block)
      block->faces.push_back(face.size());
      face.push_back(f);
    }

    {
      Face* f = new Face();
      f->v.push_back(vmpp);
      f->v.push_back(vmpm);
      f->v.push_back(vmmp);
      f->color_w = opaque; 
      f->setnormal();
      f->texture = m_texture;
      if(block)
      block->faces.push_back(face.size());
      face.push_back(f);
    }

    {
      Face* f = new Face();
      f->v.push_back(vmmm);
      f->v.push_back(vmmp);
      f->v.push_back(vmpm);
      f->color_w = opaque; 
      f->setnormal();
      f->texture = m_texture;
      if(block)
      block->faces.push_back(face.size());
      face.push_back(f);
    }

    {
      Face* f = new Face();
      f->v.push_back(vppm);
      f->v.push_back(vpmm);
      f->v.push_back(vmpm);
      f->color_w = opaque; 
      f->setnormal();
      f->texture = m_texture;
      if(block)
      block->faces.push_back(face.size());
      face.push_back(f);
    }

    {
      Face* f = new Face();
      f->v.push_back(vmmm);
      f->v.push_back(vmpm);
      f->v.push_back(vpmm);
      f->color_w = opaque; 
      f->setnormal();
      f->texture = m_texture;
      if(block)
      block->faces.push_back(face.size());
      face.push_back(f);
    }

    {
      Face* f = new Face();
      f->v.push_back(vppp);
      f->v.push_back(vpmp);
      f->v.push_back(vppm);
      f->color_w = opaque; 
      f->setnormal();
      f->texture = m_texture;
      if(block)
      block->faces.push_back(face.size());
      face.push_back(f);
    }

    {
      Face* f = new Face();
      f->v.push_back(vpmm);
      f->v.push_back(vppm);
      f->v.push_back(vpmp);
      f->color_w = opaque; 
      f->setnormal();
      f->texture = m_texture;
      if(block)
      block->faces.push_back(face.size());
      face.push_back(f);
    }

    {
      Face* f = new Face();
      f->v.push_back(vppp);
      f->v.push_back(vppm);
      f->v.push_back(vmpp);
      f->color_w = opaque; 
      f->setnormal();
      f->texture = m_texture;
      if(block)
      block->faces.push_back(face.size());
      face.push_back(f);
    }

    {
      Face* f = new Face();
      f->v.push_back(vmpm);
      f->v.push_back(vmpp);
      f->v.push_back(vppm);
      f->color_w = opaque; 
      f->setnormal();
      f->texture = m_texture;
      if(block)
      block->faces.push_back(face.size());
      face.push_back(f);
    }

    {
      Face* f = new Face();
      f->v.push_back(vpmp);
      f->v.push_back(vmmp);
      f->v.push_back(vpmm);
      f->color_w = opaque; 
      f->setnormal();
      f->texture = m_texture;
      if(block)
      block->faces.push_back(face.size());
      face.push_back(f);
    }

    {
      Face* f = new Face();
      f->v.push_back(vmmm);
      f->v.push_back(vpmm);
      f->v.push_back(vmmp);
      f->color_w = opaque; 
      f->setnormal();
      f->texture = m_texture;
      if(block)
      block->faces.push_back(face.size());
      face.push_back(f);
    }





    }

};



Block **** data;

class SuperVertex
{

public:

    std::vector<Vertex*> points;

    std::set<SuperVertex*> edge;

    float x,y,z;

};

class MeshSmoother
{

public:

    std::map<int,SuperVertex*> super_vertex;

    int width;

    MeshSmoother()
    {
        width = 1000;
    }

    void StripMesh(Mesh* mesh) // get all surface touching vertices and store into nearest super_vertices
    {

        if(mesh)
        {

            for(int i=0;i<mesh->face.size();i++)
            {

                Face* f = mesh->face[i];

                for(int j=0;j<f->v.size();j++)
                {

                    Vertex* v = f->v[j];
                    int x = v->p[0] + 50.5;
                    int y = v->p[1] + 50.5;
                    int z = v->p[2] + 50.5;

                    if(x > 1 && x < 99 && y > 1 && y < 99 && z > 1 && z < 99)

                    if( data[(int)(x-1)][(int)(y-1)][(int)(z-1)] == NULL
                     || data[(int)(x-1)][(int)(y-1)][(int)(z)] == NULL
                     || data[(int)(x-1)][(int)(y-1)][(int)(z+1)] == NULL
                     || data[(int)(x-1)][(int)(y)][(int)(z-1)] == NULL
                     || data[(int)(x-1)][(int)(y)][(int)(z)] == NULL
                     || data[(int)(x-1)][(int)(y)][(int)(z+1)] == NULL
                     || data[(int)(x-1)][(int)(y+1)][(int)(z-1)] == NULL
                     || data[(int)(x-1)][(int)(y+1)][(int)(z)] == NULL
                     || data[(int)(x-1)][(int)(y+1)][(int)(z+1)] == NULL

                     || data[(int)(x)][(int)(y-1)][(int)(z-1)] == NULL
                     || data[(int)(x)][(int)(y-1)][(int)(z)] == NULL
                     || data[(int)(x)][(int)(y-1)][(int)(z+1)] == NULL
                     || data[(int)(x)][(int)(y)][(int)(z-1)] == NULL
                   //|| data[(int)(x)][(int)(y)][(int)(z)] == NULL
                     || data[(int)(x)][(int)(y)][(int)(z+1)] == NULL
                     || data[(int)(x)][(int)(y+1)][(int)(z-1)] == NULL
                     || data[(int)(x)][(int)(y+1)][(int)(z)] == NULL
                     || data[(int)(x)][(int)(y+1)][(int)(z+1)] == NULL

                     || data[(int)(x+1)][(int)(y-1)][(int)(z-1)] == NULL
                     || data[(int)(x+1)][(int)(y-1)][(int)(z)] == NULL
                     || data[(int)(x+1)][(int)(y-1)][(int)(z+1)] == NULL
                     || data[(int)(x+1)][(int)(y)][(int)(z-1)] == NULL
                     || data[(int)(x+1)][(int)(y)][(int)(z)] == NULL
                     || data[(int)(x+1)][(int)(y)][(int)(z+1)] == NULL
                     || data[(int)(x+1)][(int)(y+1)][(int)(z-1)] == NULL
                     || data[(int)(x+1)][(int)(y+1)][(int)(z)] == NULL
                     || data[(int)(x+1)][(int)(y+1)][(int)(z+1)] == NULL

                      ) // condition
                    {

                        v->face = f;
 
                        int identifier = (((int)x+width) * width + ((int)y+width) ) * width + (int)z+width;

                        std::map<int,SuperVertex*>::iterator it = super_vertex.find(identifier);

                        if(it != super_vertex.end())
                        {
                            it->second->points.push_back(v);
                        }
                        else
                        {
                            SuperVertex* super = new SuperVertex();
                            super->x = x;
                            super->y = y;
                            super->z = z;
                            super->points.push_back(v);
                            super_vertex[identifier] = super;
                        }

                    }

                }

            }

        }

    }

    void FindEdges(Mesh* mesh, SuperVertex* super)
    {
        if(mesh && super)
        {
            for(int i=0;i<super->points.size();i++)
            {
                Vertex* v = super->points[i];
                Face* f = v->face;
                for(int j=0;j<f->v.size();j++)
                {
                    if(v != f->v[j])
                    {
                        Vertex* w = f->v[j];
                        int x = w->p[0] + 50.5;
                        int y = w->p[1] + 50.5;
                        int z = w->p[2] + 50.5;
                        int identifier = (((int)x+width) * width + ((int)y+width) ) * width + (int)z+width;
                        std::map<int,SuperVertex*>::iterator it = super_vertex.find(identifier);
                        if(it != super_vertex.end())
                        {
                            if(it->second != super)
                            {
                                super->edge.insert(it->second);
                            }
                        }
                    }
                }
            }
        }
    }

public:

    void LoadMesh(Mesh* mesh)
    {
        if(mesh)
        {
            StripMesh(mesh);

            std::map<int,SuperVertex*>::iterator it = super_vertex.begin();
            while(it != super_vertex.end())
            {
                FindEdges(mesh,it->second);
                ++it;
            }

        }
    }

    void Smooth(Mesh* mesh,float alpha)
    {
        if(mesh)
        {
            std::map<int,SuperVertex*>::iterator it = super_vertex.begin();
            while(it != super_vertex.end())
            {
                float avg_x = 0;
                float avg_y = 0;
                float avg_z = 0;
                float x = 0;
                float y = 0;
                float z = 0;
                int size = 0;
                std::set<SuperVertex*>::iterator it_edge = it->second->edge.begin();
                while(it_edge != it->second->edge.end())
                {
                    avg_x += (*it_edge)->x-50.5;
                    avg_y += (*it_edge)->y-50.5;
                    avg_z += (*it_edge)->z-50.5;
                    ++size;
                    ++it_edge;
                }
                avg_x /= size;
                avg_y /= size;
                avg_z /= size;
                x = alpha*avg_x + (1.0-alpha)*(it->second->x-50.5);
                y = alpha*avg_y + (1.0-alpha)*(it->second->y-50.5);
                z = alpha*avg_z + (1.0-alpha)*(it->second->z-50.5);
                for(int i=0;i<it->second->points.size();i++)
                {
                    it->second->points[i]->p[0] = x;
                    it->second->points[i]->p[1] = y;
                    it->second->points[i]->p[2] = z;
                }
                it->second->x = x+50.5;
                it->second->y = y+50.5;
                it->second->z = z+50.5;
                ++it;
            }

            for(int i=0;i<mesh->face.size();i++)
            {

                Face* f = mesh->face[i];

                f->setnormal();

            }
        }
    }

};





class CVert                           // Vertex Class
{
public:
  GLfloat x;                          // X Component
  GLfloat y;                          // Y Component
  GLfloat z;                          // Z Component
};
typedef CVert CVec;                       // The Definitions Are Synonymous

class CVert4                          // Vertex Class
{
public:
  GLfloat x;                          // X Component
  GLfloat y;                          // Y Component
  GLfloat z;                          // Z Component
  GLfloat w;                          // W Component
};
typedef CVert4 CVec4;                       // The Definitions Are Synonymous


class CTexCoord                         // Texture Coordinate Class
{
public:
  GLfloat u;                          // U Component
  GLfloat v;                          // V Component
};

class CMesh
{
public:
  // Mesh Data
  char*         data;
  int           m_nVertexCount;             // Vertex Count
  CVert*        m_pVertices;                // Vertex Data
  CVert*        m_pNormals;                // Normal Data
  CVert4*       m_pColors;                // Color Data
  CTexCoord*    m_pTexCoords;               // Texture Coordinates
  unsigned int  m_nTextureId;               // Texture ID

  // Vertex Buffer Object Names
  unsigned int  m_nVBOVertices;               // Vertex VBO Name
  unsigned int  m_nVBONormals;               // Vertex VBO Name
  unsigned int  m_nVBOTexCoords;              // Texture Coordinate VBO Name
  unsigned int  m_nVBOColors;               // Color VBO Name

  bool          init;

public:

CMesh();

void destroy();

bool LoadMesh(Mesh* mesh,int factor);


void BuildVBOs();

void UpdateVBOs();

};

void DrawMesh(CMesh* g_pMesh);


// generate explosions
#if 0
class ExplosionGenerator
{
    /* A particle */
    
    struct particleData
    {
      float   position[3];
      float   speed[3];
      float   color[3];
    };
    typedef struct particleData    particleData;
    
    
    /* A piece of debris */
    
    struct debrisData
    {
      float   position[3];
      float   speed[3];
      float   orientation[3];        /* Rotation angles around x, y, and z axes */
      float   orientationSpeed[3];
      float   color[3];
      float   scale[3];
    };
    typedef struct debrisData    debrisData;
    
    
    /* Globals */
    
    particleData *   particles;
    debrisData   *   debris;   
    int              fuel;                /* "fuel" of the explosion */

    int NUM_PARTICLES;
    int NUM_DEBRIS;

    int wantNormalize;

    ExplosionGenerator ()
    {
        NUM_PARTICLES = 10000;
        NUM_DEBRIS = 700;
        wantNormalize = 1;
        particleData = new particles[NUM_PARTICLES];
        debrisData = new debris[NUM_DEBRIS];
        int fuel = 0;
    }

    /*
     * newSpeed
     *
     * Randomize a new speed vector.
     *
     */
    
    void
    newSpeed (float dest[3])
    {
      float    x;
      float    y;
      float    z;
      float    len;
    
      x = (2.0 * ((GLfloat) rand ()) / ((GLfloat) RAND_MAX)) - 1.0;
      y = (2.0 * ((GLfloat) rand ()) / ((GLfloat) RAND_MAX)) - 1.0;
      z = (2.0 * ((GLfloat) rand ()) / ((GLfloat) RAND_MAX)) - 1.0;
    
      /*
       * Normalizing the speed vectors gives a "fireball" effect
       *
       */
    
      if (wantNormalize)
        {
          len = sqrt (x * x + y * y + z * z);
    
          len *= (1.0 * ((GLfloat) rand ()) / ((GLfloat) RAND_MAX)) + 1.0;
    
          if (len)
        {
          x = x / len;
          y = y / len;
          z = z / len;
        }
        }
    
    
      dest[0] = x;
      dest[1] = y;
      dest[2] = z;
    }

    /*
     * newExplosion
     *
     * Create a new explosion.
     *
     */
    
    void
    newExplosion (void)
    {
      int    i;
    
      for (i = 0; i < NUM_PARTICLES; i++)
        {
          particles[i].position[0] = 0.0;
          particles[i].position[1] = 0.0;
          particles[i].position[2] = 0.0;
    
          particles[i].color[0] = 1.0;
          particles[i].color[1] = 1.0;
          particles[i].color[2] = 0.5;
    
          newSpeed (particles[i].speed);
        }
    
      for (i = 0; i < NUM_DEBRIS; i++)
        {
          debris[i].position[0] = 0.0;
          debris[i].position[1] = 0.0;
          debris[i].position[2] = 0.0;
    
          debris[i].orientation[0] = 0.0;
          debris[i].orientation[1] = 0.0;
          debris[i].orientation[2] = 0.0;
    
          debris[i].color[0] = 0.7;
          debris[i].color[1] = 0.7;
          debris[i].color[2] = 0.7;
    
          debris[i].scale[0] = 0.5 * ( (2.0 * ((GLfloat) rand ()) / ((GLfloat) RAND_MAX)) - 1.0 );
          debris[i].scale[1] = 0.5 * ( (2.0 * ((GLfloat) rand ()) / ((GLfloat) RAND_MAX)) - 1.0 );
          debris[i].scale[2] = 0.5 * ( (2.0 * ((GLfloat) rand ()) / ((GLfloat) RAND_MAX)) - 1.0 );
    
          newSpeed (debris[i].speed);
          newSpeed (debris[i].orientationSpeed);
        }
      
      fuel = 1000;
    }


/*
 * display
 *
 * Draw the scene.
 *
 */

void
display (void)
{
  int    i;

  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glLoadIdentity ();

  /* Place the camera */

  glTranslatef (0.0, 0.0, -10.0);
  glRotatef (angle, 0.0, 1.0, 0.0);

  /* If no explosion, draw cube */
/*
  if (fuel == 0)
    {
      glEnable (GL_LIGHTING);
      glDisable (GL_LIGHT0);
      glEnable (GL_DEPTH_TEST);
      glutSolidCube (1.0);
    }
*/
  if (fuel > 0)
    {
      glPushMatrix ();

      glDisable (GL_LIGHTING);
      glDisable (GL_DEPTH_TEST);

      glBegin (GL_POINTS);

      for (i = 0; i < NUM_PARTICLES; i++)
	{
	  glColor3fv (particles[i].color);
	  glVertex3fv (particles[i].position);
	}
  
      glEnd ();

      glPopMatrix ();

      glEnable (GL_LIGHTING); 
      glEnable (GL_LIGHT0); 
      glEnable (GL_DEPTH_TEST);

      glNormal3f (0.0, 0.0, 1.0);

      for (i = 0; i < NUM_DEBRIS; i++)
	{
	  glColor3fv (debris[i].color);

	  glPushMatrix ();
      
	  glTranslatef (debris[i].position[0],
			debris[i].position[1],
			debris[i].position[2]);

	  glRotatef (debris[i].orientation[0], 1.0, 0.0, 0.0);
	  glRotatef (debris[i].orientation[1], 0.0, 1.0, 0.0);
	  glRotatef (debris[i].orientation[2], 0.0, 0.0, 1.0);

	  glScalef (debris[i].scale[0],
		    debris[i].scale[1],
		    debris[i].scale[2]);

	  glBegin (GL_TRIANGLES);
	  glVertex3f (0.0, 0.5, 0.0);
	  glVertex3f (-0.25, 0.0, 0.0);
	  glVertex3f (0.25, 0.0, 0.0);
	  glEnd ();	  
	  
	  glPopMatrix ();
	}
    }

  glutSwapBuffers ();
}

/*
 * idle
 *
 * Update animation variables.
 *
 */

void
idle (void)
{
  int    i;

  if (!wantPause)
    {
      if (fuel > 0)
	{
	  for (i = 0; i < NUM_PARTICLES; i++)
	    {
	      particles[i].position[0] += particles[i].speed[0] * 0.2;
	      particles[i].position[1] += particles[i].speed[1] * 0.2;
	      particles[i].position[2] += particles[i].speed[2] * 0.2;
	      
	      particles[i].color[0] -= 1.0 / 500.0;
	      if (particles[i].color[0] < 0.0)
		{
		  particles[i].color[0] = 0.0;
		}
	      
	      particles[i].color[1] -= 1.0 / 100.0;
	      if (particles[i].color[1] < 0.0)
		{
		  particles[i].color[1] = 0.0;
		}
	      
	      particles[i].color[2] -= 1.0 / 50.0;
	      if (particles[i].color[2] < 0.0)
		{
		  particles[i].color[2] = 0.0;
		}
	    }
	  
	  for (i = 0; i < NUM_DEBRIS; i++)
	    {
	      debris[i].position[0] += debris[i].speed[0] * 0.1;
	      debris[i].position[1] += debris[i].speed[1] * 0.1;
	      debris[i].position[2] += debris[i].speed[2] * 0.1;
	      
	      debris[i].orientation[0] += debris[i].orientationSpeed[0] * 10;
	      debris[i].orientation[1] += debris[i].orientationSpeed[1] * 10;
	      debris[i].orientation[2] += debris[i].orientationSpeed[2] * 10;
	    }
	  
	  --fuel;
	}
      
      //angle += 0.3;  /* Always continue to rotate the camera */
    }
  
  glutPostRedisplay ();
}







};
#endif


// generate sun

const int MAX_PARTICLES = 20;

particles particle[MAX_PARTICLES];

static GLfloat colors[12][3]=	// Rainbow Of Colors
{
  {1.0f,0.0f ,0.0f}
, {1.0f,0.08f,0.1f}
, {1.0f,0.16f,0.2f}
, {1.0f,0.24f,0.4f}

, {1.0f,0.32f,0.6f}
, {1.0f,0.40f,0.8f}
, {0.8f,0.48f,1.0f}
, {0.6f,0.56f,1.0f}

, {0.4f,0.64f,1.0f}
, {0.2f,0.72f,1.0f}
, {0.1f,0.80f,1.0f}
, {0.0f,0.88f,1.0f}
};



class SunGenerator
{

public:

    GLfloat rot_m[16];
    CMesh* cmesh;
    Mesh* mesh;

    float slowdown;

    GLuint loop;
    GLuint col;
    GLuint delay;
    //GLuint* texture;

    SunGenerator()
    {
        slowdown = 10.0f;
        cmesh = new CMesh();
        mesh = new Mesh();

/*
        mesh->AddBox( 40
                    , 40
                    , 40
                    , 0.5
                    , 0.5
                    , 0.5
                    , 1
                    );
*/

        
        int num = 100;
        float width = 1000.0f;
        for(int i=0;i<num;i++)
        {

        float x = 0;//1*(.5-(rand()%1000)/1000.0);
        float y = 0;//1*(.5-(rand()%1000)/1000.0);
        {
          Face* f = new Face();
          f->v.push_back(new Vertex(x+width*cos(2*3.14159*i/num)+width*sin(2*3.14159*i/num),y+width*cos(2*3.14159*i/num)-width*sin(2*3.14159*i/num),-i*0.005*width));
          f->v.push_back(new Vertex(x-width*cos(2*3.14159*i/num)+width*sin(2*3.14159*i/num),y+width*cos(2*3.14159*i/num)+width*sin(2*3.14159*i/num),-i*0.005*width));
          f->v.push_back(new Vertex(x+width*cos(2*3.14159*i/num)-width*sin(2*3.14159*i/num),y-width*cos(2*3.14159*i/num)-width*sin(2*3.14159*i/num),-i*0.005*width));
          f->setnormal();
          f->texture = 1;
          mesh->face.push_back(f);
        }

        {
          Face* f = new Face();
          f->v.push_back(new Vertex(x-width*cos(2*3.14159*i/num)-width*sin(2*3.14159*i/num),y-width*cos(2*3.14159*i/num)+width*sin(2*3.14159*i/num),-i*0.005*width));
          f->v.push_back(new Vertex(x+width*cos(2*3.14159*i/num)-width*sin(2*3.14159*i/num),y-width*cos(2*3.14159*i/num)-width*sin(2*3.14159*i/num),-i*0.005*width));
          f->v.push_back(new Vertex(x-width*cos(2*3.14159*i/num)+width*sin(2*3.14159*i/num),y+width*cos(2*3.14159*i/num)+width*sin(2*3.14159*i/num),-i*0.005*width));
          f->setnormal();
          f->texture = 1;
          mesh->face.push_back(f);
        }

        }


        for(int i=0;i<mesh->face.size();i++)
        {
            mesh->face[i]->color_r = 1.0;
            mesh->face[i]->color_g = 1.0;
            mesh->face[i]->color_b = 1.0;
            mesh->face[i]->color_w = 3.0f/num;
        }

    }

    int LoadGLTextures()							// Load Bitmap And Convert To A Texture
    {
/* 
    	int Status;
    	GLubyte *tex = new GLubyte[32 * 32 * 3];
    	FILE *tf;
    
    	tf = fopen ( "Particle.raw", "rb" );
    	int ret = fread ( tex, 1, 32 * 32 * 3, tf );
    	fclose ( tf );
    
    	// do stuff
    	Status=1;						// Set The Status To TRUE
        //texture = new GLuint[1];
    	//glGenTextures(1, &texture[0]);				// Create One Texture
    
    	//glBindTexture(GL_TEXTURE_2D, texture[0]);
    	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    	glTexImage2D(GL_TEXTURE_2D, 0, 3, 32, 32,
    		0, GL_RGB, GL_UNSIGNED_BYTE, tex);
    
    	delete [] tex;
    
            return Status;							// Return The Status
*/
        return 0;
    }

    void initialize()
    {
/* 

	    for (loop=0;loop<MAX_PARTICLES;loop++)				// Initials All The Textures
	    {
            //std::cout << "loop=" << loop << std::endl;
	    	particle[loop].active=false;				// Make All The Particles Active
	    	particle[loop].life=1.0f;//0.1f;				// Give All The Particles Full Life
	    	particle[loop].fade=float(rand()%100)/1000.0f+0.003f;	// Random Fade Speed
	    	particle[loop].r=colors[(loop+1)/(MAX_PARTICLES/12)][0];	// Select Red Rainbow Color
	    	particle[loop].g=colors[(loop+1)/(MAX_PARTICLES/12)][1];	// Select Green Rainbow Color
	    	particle[loop].b=colors[(loop+1)/(MAX_PARTICLES/12)][2];	// Select Blue Rainbow Color
            do
            {
	    	    particle[loop].xi=float((rand()%500)-250.0f)*500.0f;	// Random Speed On X Axis
	    	    particle[loop].yi=float((rand()%500)-250.0f)*500.0f;	// Random Speed On Y Axis
	    	    particle[loop].zi=float((rand()%500)-250.0f)*500.0f;	// Random Speed On Z Axis
            }
            while(sqrt(particle[loop].xi*particle[loop].xi+particle[loop].yi*particle[loop].yi+particle[loop].zi*particle[loop].zi) > 250*500.0f);
	    	particle[loop].xg=0.0f;					// Set Horizontal Pull To Zero
	    	//particle[loop].yg=-0.4f;				// Set Vertical Pull Downward
	    	particle[loop].yg=-0.0f;				// Set Vertical Pull Downward
	    	particle[loop].zg=0.0f;					// Set Pull On Z Axis To Zero
	    }
        std::cout << "done initialize" << std::endl;
*/
    }

    void draw()
    {
/* 
        //if(rand()%50 < 10)
        float sx   = 40;
        float sy   = 40;
        float zoom = 40;
        static bool init = true;
        if(init)
        {
            init = false;
            //float sx = (rand()%200-100);
            //float sy = (rand()%200-100);
            for(loop=0;loop<MAX_PARTICLES;loop++)
            {
                do
                {
	    	        particle[loop].xi=float((rand()%500)-250.0f)*500.0f;	// Random Speed On X Axis
	    	        particle[loop].yi=float((rand()%500)-250.0f)*500.0f;	// Random Speed On Y Axis
	    	        particle[loop].zi=float((rand()%500)-250.0f)*500.0f;	// Random Speed On Z Axis
                }
                while(sqrt(particle[loop].xi*particle[loop].xi+particle[loop].yi*particle[loop].yi+particle[loop].zi*particle[loop].zi) > 250*500.0f);

    			particle[loop].x=sx     +particle[loop].xi/(slowdown*1000);					// Center On X Axis
    			particle[loop].y=sy     +particle[loop].yi/(slowdown*1000);					// Center On Y Axis
    			particle[loop].z=zoom   +particle[loop].zi/(slowdown*1000);					// Center On Z Axis
                particle[loop].active = true;
            }
        }

        float x,y,z;
        float xi,yi,zi;
        int i;   

        static int time = 0;
        time++;
        int delay = 10;
    	for (loop=0;loop<MAX_PARTICLES;loop++)				// Loop Through All The Particles
    	{
    		if (particle[loop].active)				// If The Particle Is Active
    		{
    			x=particle[loop].x;			// Grab Our Particle X Position
    			y=particle[loop].y;			// Grab Our Particle Y Position
    			z=particle[loop].z;			// Particle Z Pos + Zoom
    			xi=particle[loop].xi/(slowdown*1000);			// Grab Our Particle X Position
    			yi=particle[loop].yi/(slowdown*1000);			// Grab Our Particle Y Position
    			zi=particle[loop].zi/(slowdown*1000);			// Particle Z Pos + Zoom
    
    			// Draw The Particle Using Our RGB Values, Fade The Particle Based On It's Life
    			//glColor4f(particle[loop].r,particle[loop].g,particle[loop].b,particle[loop].life);
   
 
    			//glBegin(GL_QUADS);				// Build Quad From A Triangle Strip
    			//    glTexCoord2d(1,1); glVertex3f(x+35.0f,y+35.0f,z);	// Top Right
    			//	glTexCoord2d(0,1); glVertex3f(x-35.0f,y+35.0f,z); // Top Left
    			//	glTexCoord2d(0,0); glVertex3f(x-35.0f,y-35.0f,z); // Bottom Left
    			//	glTexCoord2d(1,0); glVertex3f(x+35.0f,y-35.0f,z); // Bottom Right
                //glEnd();

                if(time > delay)
                particle[loop].life = 1.0f;// * (0.03f+0.01f*((rand()%1000)/1000.0f));
    
                //particle[loop].xi *= .995;
                //particle[loop].yi *= .995;
                //particle[loop].zi *= .995;
    
    			//particle[loop].x+=xi;// Move On The X Axis By X Speed
    			//particle[loop].y+=yi;// Move On The Y Axis By Y Speed
    			//particle[loop].z+=zi;// Move On The Z Axis By Z Speed
    
    			//particle[loop].xi+=particle[loop].xg;			// Take Pull On X Axis Into Account
    			//particle[loop].yi+=particle[loop].yg;			// Take Pull On Y Axis Into Account
    			//particle[loop].zi+=particle[loop].zg;			// Take Pull On Z Axis Into Account
    			//particle[loop].life-=particle[loop].fade;		// Reduce Particles Life By 'Fade'
    
    			if (particle[loop].life<0.0f)					// If Particle Is Burned Out
    			{
                    col = (col+1)%12;
    				particle[loop].life=1.0f;				// Give It New Life
    				particle[loop].fade=float(rand()%100)/1000.0f+0.003f;	// Random Fade Value
    				particle[loop].x=0.0f;					// Center On X Axis
    				particle[loop].y=0.0f;					// Center On Y Axis
    				particle[loop].z=0.0f;					// Center On Z Axis
                    do
                    {
                        float xspeed = 0;
                        float yspeed = 0;
    				    particle[loop].xi=xspeed+float((rand()%500)-250.0f)*100.0f;	// X Axis Speed And Direction
    				    particle[loop].yi=yspeed+float((rand()%500)-250.0f)*100.0f;	// Y Axis Speed And Direction
    				    particle[loop].zi=       float((rand()%500)-250.0f)*100.0f;	// Z Axis Speed And Direction
                    }
                    while(sqrt(particle[loop].xi*particle[loop].xi+particle[loop].yi*particle[loop].yi+particle[loop].zi*particle[loop].zi) > 25000.0f);
                    //if(rand()%300==0)
                    //{
                    //    particle[loop].xi *= 5;
                    //    particle[loop].yi *= 5;
                    //    particle[loop].zi *= 5;
                    //    particle[loop].fade /= 5;
                    //}
                    //else
                    //if(rand()%5==0)
                    //{
                    //    float vx = particle[loop].xi;
                    //    float vy = particle[loop].yi;
                    //    particle[loop].xi = 1750.0f*vx/sqrt(vx*vx+vy*vy);
                    //    particle[loop].yi = 1750.0f*vy/sqrt(vx*vx+vy*vy);
                    //    particle[loop].zi = 0.0f;
                    //    particle[loop].fade *= 2;
                    //}
    				particle[loop].r=colors[col][0];			// Select Red From Color Table
    				particle[loop].g=colors[col][1];			// Select Green From Color Table
    				particle[loop].b=colors[col][2];			// Select Blue From Color Table
                    particle[loop].active = false;
    			}
   
    		}
       	}

        if(time > delay){
            time = 0; 
        }
*/



        static bool init_2 = true;
        if(init_2)
        {
        init_2 = false;
        cmesh->LoadMesh(mesh,1.0f);
        cmesh->BuildVBOs();
        }
        // look = camera_pos - point_pos
        float look_x = xPosition - -4000.0;
        float look_y = yPosition - 0.0;
        float look_z = zPosition - 0.0;
        float look_r = sqrt(look_x*look_x+look_y*look_y+look_z*look_z);
        // up = camera up vector
        float up_x = cameraRotation[1];
        float up_y = cameraRotation[5];
        float up_z = cameraRotation[9];
        float up_r;
        // right = up x look
        float right_x = up_y*look_z - up_z*look_y;
        float right_y = up_z*look_x - up_x*look_z;
        float right_z = up_x*look_y - up_y*look_x;
        float right_r = sqrt(right_x*right_x+right_y*right_y+right_z*right_z);
        // up = look x right
        up_x = look_y*right_z - look_z*right_y;
        up_y = look_z*right_x - look_x*right_z;
        up_z = look_x*right_y - look_y*right_x;
        up_r = sqrt(up_x*up_x+up_y*up_y+up_z*up_z);
        // normalize
        look_x /= look_r;
        look_y /= look_r;
        look_z /= look_r;
        up_x /= up_r;
        up_y /= up_r;
        up_z /= up_r;
        right_x /= right_r;
        right_y /= right_r;
        right_z /= right_r;

        glPushMatrix();
        glTranslatef(xPosition-4000.0,yPosition,zPosition);
        rot_m[0]  = right_x; rot_m[1]  = right_y; rot_m[2]  = right_z; rot_m[3]  = 0 ;
        rot_m[4]  = up_x   ; rot_m[5]  = up_y   ; rot_m[6]  = up_z   ; rot_m[7]  = 0 ;
        rot_m[8]  = -look_x; rot_m[9]  = -look_y; rot_m[10] = -look_z; rot_m[11] = 0 ;
        rot_m[12] = 0      ; rot_m[13] = 0      ; rot_m[14] = 0      ; rot_m[15] = 1 ;
        glMultMatrixf(rot_m);
    	glBindTexture(GL_TEXTURE_2D, texture[3]);
        glEnable(GL_BLEND);
        DrawMesh(cmesh);
        glDisable(GL_BLEND);
        glPopMatrix();

    }

};

// done generating sun

SunGenerator* sun_generator = NULL;




CMesh::CMesh()
{
  data = NULL;
  m_pVertices = NULL;
  m_pNormals = NULL;
  m_pTexCoords = NULL;
  m_pColors = NULL;
  m_nVertexCount = 0;
  m_nVBOVertices = m_nVBONormals = m_nVBOTexCoords = m_nTextureId = m_nVBOColors = 0;
  init = true;
}

void CMesh::destroy()
{
  {
    unsigned int nBuffers[4] = { m_nVBOVertices, m_nVBONormals, m_nVBOTexCoords, m_nVBOColors };
    glDeleteBuffersARB( 4, nBuffers );            // Free The Memory
  }
/* 
  if( m_pVertices )                     // Deallocate Vertex Data
    delete [] m_pVertices;
  m_pVertices = NULL;
  if( m_pNormals )
    delete [] m_pNormals;
  m_pNormals = NULL;
  if( m_pTexCoords )                      // Deallocate Texture Coord Data
    delete [] m_pTexCoords;
  m_pTexCoords = NULL;
  if( m_pColors )                      // Deallocate Texture Coord Data
    delete [] m_pColors;
  m_pColors = NULL;
*/

  if(m_nVBOTexCoords)                 // Deallocate Vertex Buffer
    glDeleteBuffersARB( 1, &m_nVBOTexCoords);
  if(m_nVBONormals)
    glDeleteBuffersARB( 1, &m_nVBONormals );
  if(m_nVBOVertices)
    glDeleteBuffersARB( 1, &m_nVBOVertices );
  if(m_nVBOColors)
    glDeleteBuffersARB( 1, &m_nVBOColors );
 
//  if(data)
//    delete [] data;

}

bool CMesh::LoadMesh( Mesh* mesh , int factor = 8 )
{

  ////std::cout << "mesh faces=" << mesh->face.size() << std::endl;

  if(mesh->face.size() > 0){

  int sqrt_vertex = ((int)(sqrt(mesh->face.size())) + 1);

  // Generate Vertex Field
  m_nVertexCount = (int)(3000000);
  if(m_pVertices == NULL)
    m_pVertices = new CVec[m_nVertexCount];           // Allocate Vertex Data
  if(m_pNormals == NULL)
    m_pNormals = new CVec[m_nVertexCount];
  if(m_pTexCoords == NULL)
    m_pTexCoords = new CTexCoord[m_nVertexCount];       // Allocate Tex Coord Data
  if(m_pColors == NULL)
    m_pColors = new CVec4[m_nVertexCount];       // Allocate Tex Coord Data
  
  for(int nIndex=0;nIndex<m_nVertexCount&&nIndex<mesh->face.size()*3;nIndex+=3){
    m_pVertices[nIndex+0].x = mesh->face[nIndex/3]->v[0]     ->p[0];
    m_pVertices[nIndex+0].y = mesh->face[nIndex/3]->v[0]     ->p[1];
    m_pVertices[nIndex+0].z = mesh->face[nIndex/3]->v[0]     ->p[2];
    m_pNormals[nIndex+0].x  = mesh->face[nIndex/3]->normal[0]. p[0];
    m_pNormals[nIndex+0].y  = mesh->face[nIndex/3]->normal[0]. p[1];
    m_pNormals[nIndex+0].z  = mesh->face[nIndex/3]->normal[0]. p[2];
    m_pColors[nIndex+0].x   = mesh->face[nIndex/3]->color_r;
    m_pColors[nIndex+0].y   = mesh->face[nIndex/3]->color_g;
    m_pColors[nIndex+0].z   = mesh->face[nIndex/3]->color_b;
    m_pColors[nIndex+0].w   = mesh->face[nIndex/3]->color_w;
    m_pVertices[nIndex+1].x = mesh->face[nIndex/3]->v[1]     ->p[0];
    m_pVertices[nIndex+1].y = mesh->face[nIndex/3]->v[1]     ->p[1];
    m_pVertices[nIndex+1].z = mesh->face[nIndex/3]->v[1]     ->p[2];
    m_pNormals[nIndex+1].x  = mesh->face[nIndex/3]->normal[0]. p[0];
    m_pNormals[nIndex+1].y  = mesh->face[nIndex/3]->normal[0]. p[1];
    m_pNormals[nIndex+1].z  = mesh->face[nIndex/3]->normal[0]. p[2];
    m_pColors[nIndex+1].x   = mesh->face[nIndex/3]->color_r;
    m_pColors[nIndex+1].y   = mesh->face[nIndex/3]->color_g;
    m_pColors[nIndex+1].z   = mesh->face[nIndex/3]->color_b;
    m_pColors[nIndex+1].w   = mesh->face[nIndex/3]->color_w;
    m_pVertices[nIndex+2].x = mesh->face[nIndex/3]->v[2]     ->p[0];
    m_pVertices[nIndex+2].y = mesh->face[nIndex/3]->v[2]     ->p[1];
    m_pVertices[nIndex+2].z = mesh->face[nIndex/3]->v[2]     ->p[2];
    m_pNormals[nIndex+2].x  = mesh->face[nIndex/3]->normal[0]. p[0];
    m_pNormals[nIndex+2].y  = mesh->face[nIndex/3]->normal[0]. p[1];
    m_pNormals[nIndex+2].z  = mesh->face[nIndex/3]->normal[0]. p[2];
    m_pColors[nIndex+2].x   = mesh->face[nIndex/3]->color_r;
    m_pColors[nIndex+2].y   = mesh->face[nIndex/3]->color_g;
    m_pColors[nIndex+2].z   = mesh->face[nIndex/3]->color_b;
    m_pColors[nIndex+2].w   = mesh->face[nIndex/3]->color_w;

    ////std::cout << mesh->face[nIndex/3].texture << std::endl;

    double inv = (1.0f/factor);
    double su = inv*((int)mesh->face[nIndex/3]->texture%factor);
    double eu = su+inv;

    double sv = inv*((int)mesh->face[nIndex/3]->texture/factor);
    double ev = sv+inv;


    if((nIndex/3)%2){

      m_pTexCoords[nIndex+1].u = su;//(double)(((nIndex/3)%sqrt_vertex + 0.0))/sqrt_vertex;
      m_pTexCoords[nIndex+1].v = ev;//(double)(((nIndex/3)/sqrt_vertex + 1.0))/sqrt_vertex;

      m_pTexCoords[nIndex+2].u = eu;//(double)(((nIndex/3)%sqrt_vertex + 1.0))/sqrt_vertex;
      m_pTexCoords[nIndex+2].v = sv;//(double)(((nIndex/3)/sqrt_vertex + 0.0))/sqrt_vertex;

      m_pTexCoords[nIndex+0].u = su;//(double)(((nIndex/3)%sqrt_vertex + 0.0))/sqrt_vertex;
      m_pTexCoords[nIndex+0].v = sv;//(double)(((nIndex/3)/sqrt_vertex + 0.0))/sqrt_vertex;

    }else{

      m_pTexCoords[nIndex+2].u = su;//(double)(((nIndex/3)%sqrt_vertex + 0.0))/sqrt_vertex;
      m_pTexCoords[nIndex+2].v = ev;//(double)(((nIndex/3)/sqrt_vertex + 1.0))/sqrt_vertex;

      m_pTexCoords[nIndex+1].u = eu;//(double)(((nIndex/3)%sqrt_vertex + 1.0))/sqrt_vertex;
      m_pTexCoords[nIndex+1].v = sv;//(double)(((nIndex/3)/sqrt_vertex + 0.0))/sqrt_vertex;

      m_pTexCoords[nIndex+0].u = eu;//(double)(((nIndex/3)%sqrt_vertex + 0.0))/sqrt_vertex;
      m_pTexCoords[nIndex+0].v = ev;//(double)(((nIndex/3)/sqrt_vertex + 0.0))/sqrt_vertex;

    }
 
  }

    }

  return true;
}

void CMesh::UpdateVBOs()
{

  glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVBOVertices);
  CVec* Vertices_ptr = (CVec*)glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);

  if(Vertices_ptr)
  {
    for(int i=0;i<m_nVertexCount;i++){
        Vertices_ptr[i]  = m_pVertices[i];
    }
    glUnmapBufferARB(GL_ARRAY_BUFFER_ARB); 
  }

  glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVBOTexCoords);
  CTexCoord* TexCoords_ptr = (CTexCoord*)glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);

  if(TexCoords_ptr)
  {
    for(int i=0;i<m_nVertexCount;i++){
        TexCoords_ptr[i] = m_pTexCoords[i];
    }
    glUnmapBufferARB(GL_ARRAY_BUFFER_ARB); 
  }

  glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVBONormals);
  CVec* Normals_ptr = (CVec*)glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);

  if(Normals_ptr)
  {
    for(int i=0;i<m_nVertexCount;i++){
        Normals_ptr[i]   = m_pNormals[i];
    }
    glUnmapBufferARB(GL_ARRAY_BUFFER_ARB); 
  }

  glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVBOColors);
  CVec4* Colors_ptr = (CVec4*)glMapBufferARB(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY_ARB);

  if(Colors_ptr)
  {
    for(int i=0;i<m_nVertexCount;i++){
        Colors_ptr[i]    = m_pColors[i];
    }
    glUnmapBufferARB(GL_ARRAY_BUFFER_ARB); 
  }



}

void CMesh::BuildVBOs()
{
  m_nVBOVertices  = 3*m_nVertexCount;
  m_nVBONormals   = 3*m_nVertexCount;
  m_nVBOTexCoords = 2*m_nVertexCount;
  m_nVBOColors    = 4*m_nVertexCount;
  //std::cout << "num_vertices=" << m_nVBOVertices << std::endl;
  // Generate And Bind The Vertex Buffer
  if(init){
  glGenBuffersARB( 1, &m_nVBOVertices );              // Get A Valid Name
  }
  glBindBufferARB( GL_ARRAY_BUFFER_ARB, m_nVBOVertices );     // Bind The Buffer
  // Load The Data
  glBufferDataARB( GL_ARRAY_BUFFER_ARB, m_nVertexCount*3*sizeof(float), m_pVertices, GL_STATIC_DRAW_ARB );

  // Generate And Bind The Normal Buffer
  if(init){
  glGenBuffersARB( 1, &m_nVBONormals );             // Get A Valid Name
  }
  glBindBufferARB( GL_ARRAY_BUFFER_ARB, m_nVBONormals );    // Bind The Buffer
  // Load The Data
  glBufferDataARB( GL_ARRAY_BUFFER_ARB, m_nVertexCount*3*sizeof(float), m_pNormals, GL_STATIC_DRAW_ARB );

  // Generate And Bind The Texture Coordinate Buffer
  if(init){
  glGenBuffersARB( 1, &m_nVBOTexCoords );             // Get A Valid Name
  }
  glBindBufferARB( GL_ARRAY_BUFFER_ARB, m_nVBOTexCoords );    // Bind The Buffer
  // Load The Data
  glBufferDataARB( GL_ARRAY_BUFFER_ARB, m_nVertexCount*2*sizeof(float), m_pTexCoords, GL_STATIC_DRAW_ARB );

  if(init){
  glGenBuffersARB(1, &m_nVBOColors);
  }
  glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_nVBOColors);
  glBufferDataARB(GL_ARRAY_BUFFER_ARB, m_nVertexCount*4*sizeof(float), m_pColors, GL_STATIC_DRAW);

  init = false;

}

void DrawMesh(CMesh* g_pMesh){

  // Enable Pointers
  glEnableClientState( GL_VERTEX_ARRAY );           // Enable Vertex Arrays
  glEnableClientState( GL_TEXTURE_COORD_ARRAY );        // Enable Texture Coord Arrays
  glEnableClientState( GL_COLOR_ARRAY );

  glBindBufferARB( GL_ARRAY_BUFFER_ARB, g_pMesh->m_nVBOVertices );
  glVertexPointer( 3, GL_FLOAT, 0, (char *) NULL );   // Set The Vertex Pointer To The Vertex Buffer
  glBindBufferARB( GL_ARRAY_BUFFER_ARB, g_pMesh->m_nVBOTexCoords );
  glTexCoordPointer( 2, GL_FLOAT, 0, (char *) NULL );   // Set The TexCoord Pointer To The TexCoord Buffer
  glBindBufferARB(GL_ARRAY_BUFFER_ARB, g_pMesh->m_nVBOColors);
  glColorPointer( 4, GL_FLOAT, 0, (char*) NULL );

  // Render
  glDrawArrays( GL_TRIANGLES, 0, g_pMesh->m_nVertexCount ); // Draw All Of The Triangles At Once

  // Disable Pointers
  glDisableClientState( GL_VERTEX_ARRAY );          // Disable Vertex Arrays
  glDisableClientState( GL_TEXTURE_COORD_ARRAY );       // Disable Texture Coord Arrays
  glDisableClientState( GL_COLOR_ARRAY );

}

Mesh mesh;

Mesh enemy_creature_mesh;

Mesh enemy_drone_mesh;

Mesh space_station_mesh;

CMesh* cmesh;

CMesh* enemy_creature_cmesh;

CMesh* enemy_drone_cmesh;

CMesh* space_station_cmesh;


void rotateMatrixAroundVector( float & m00
                             , float & m01
                             , float & m02
                             , float & m10
                             , float & m11
                             , float & m12
                             , float & m20
                             , float & m21
                             , float & m22
                             , float x
                             , float y
                             , float z
                             )
{

    float r__,s__,c__,v__;
    r__ = sqrt(x*x+y*y+z*z);
    if(r__ > 1e-50){
    	float rot[9];
        float m[9];
        x /= r__;
        y /= r__;
        z /= r__;
        s__ = sin(r__);
        c__ = cos(r__);
        v__ = 1.0-c__;
        rot[0] = (x*x*v__ + c__);
        rot[1] = (x*y*v__ - z*s__);
        rot[2] = (x*z*v__ + y*s__);
        rot[3] = (x*y*v__ + z*s__);
        rot[4] = (y*y*v__ + c__);
        rot[5] = (z*y*v__ - x*s__);
        rot[6] = (x*z*v__ - y*s__);
        rot[7] = (y*z*v__ + x*s__);
        rot[8] = (z*z*v__ + c__);
        m[0] = m00;m[1] = m10;m[2] = m20;
        m[3] = m01;m[4] = m11;m[5] = m21;
        m[6] = m02;m[7] = m12;m[8] = m22;
        m00 = rot[0]*m[0]+rot[1]*m[1]+rot[2]*m[2];
        m01 = rot[0]*m[3]+rot[1]*m[4]+rot[2]*m[5];
        m02 = rot[0]*m[6]+rot[1]*m[7]+rot[2]*m[8];
        m10 = rot[3]*m[0]+rot[4]*m[1]+rot[5]*m[2];
        m11 = rot[3]*m[3]+rot[4]*m[4]+rot[5]*m[5];
        m12 = rot[3]*m[6]+rot[4]*m[7]+rot[5]*m[8];
        m20 = rot[6]*m[0]+rot[7]*m[1]+rot[8]*m[2];
        m21 = rot[6]*m[3]+rot[7]*m[4]+rot[8]*m[5];
        m22 = rot[6]*m[6]+rot[7]*m[7]+rot[8]*m[8];
    }
 
}


float scale = 0.5f;


int start_color = 123456;

GLfloat asteroid_wx = 0.00004;
GLfloat asteroid_wy = 0.00004;
GLfloat asteroid_wz = 0.00004;

void find_peripherals(Block* b,std::map<std::string,std::vector<Block*> >& p_map){
    int color = ++start_color;
    std::stack<Block*> Q;
    Block* t;
    p_map.clear();
    Q.push(b);
    while(Q.size() > 0){
        t = Q.top();
        Q.pop();
        {
            t->color = color;
            if(t->type == 16){
                std::string group = t->menu->component[1]->name;
                if(p_map.find(group) != p_map.end()) {
                    p_map[group].push_back(t);
                } else {
                    p_map[group] = std::vector<Block*>();
                    p_map[group].push_back(t);
                }
            }
            if(t->x+1<100)
            if(data         [t->x+1][t->y][t->z]!=NULL)
            if(data         [t->x+1][t->y][t->z]->type == 9
            || data         [t->x+1][t->y][t->z]->type == 16
              )
            if(data         [t->x+1][t->y][t->z]->color != color){
            data            [t->x+1][t->y][t->z]->color = color;
            Q.push(data     [t->x+1][t->y][t->z]);
            }
            if(t->x-1>=0)
            if(data         [t->x-1][t->y][t->z]!=NULL)
            if(data         [t->x-1][t->y][t->z]->type == 9
            || data         [t->x-1][t->y][t->z]->type == 16
              )
            if(data         [t->x-1][t->y][t->z]->color != color){
            data            [t->x-1][t->y][t->z]->color = color;
            Q.push(data     [t->x-1][t->y][t->z]);
            }
            if(t->y+1<100)
            if(data         [t->x][t->y+1][t->z]!=NULL)
            if(data         [t->x][t->y+1][t->z]->type == 9
            || data         [t->x][t->y+1][t->z]->type == 16
              )
            if(data         [t->x][t->y+1][t->z]->color != color){
            data            [t->x][t->y+1][t->z]->color = color;
            Q.push(data     [t->x][t->y+1][t->z]);
            }
            if(t->y-1>=0)
            if(data         [t->x][t->y-1][t->z]!=NULL)
            if(data         [t->x][t->y-1][t->z]->type == 9
            || data         [t->x][t->y-1][t->z]->type == 16
              )
            if(data         [t->x][t->y-1][t->z]->color != color){
            data            [t->x][t->y-1][t->z]->color = color;
            Q.push(data     [t->x][t->y-1][t->z]);
            }
            if(t->z+1<100)
            if(data         [t->x][t->y][t->z+1]!=NULL)
            if(data         [t->x][t->y][t->z+1]->type == 9
            || data         [t->x][t->y][t->z+1]->type == 16
              )
            if(data         [t->x][t->y][t->z+1]->color != color){
            data            [t->x][t->y][t->z+1]->color = color;
            Q.push(data     [t->x][t->y][t->z+1]);
            }
            if(t->z-1>=0)
            if(data         [t->x][t->y][t->z-1]!=NULL)
            if(data         [t->x][t->y][t->z-1]->type == 9
            || data         [t->x][t->y][t->z-1]->type == 16
              )
            if(data         [t->x][t->y][t->z-1]->color != color){
            data            [t->x][t->y][t->z-1]->color = color;
            Q.push(data     [t->x][t->y][t->z-1]);
            }
        }
    }
}

void FindPeripheralsSignal::execute(void* arg){
    Menu* m = (Menu*)arg;
    std::map<std::string,std::vector<Block*> > vec;
    if(m->block_parent){
    find_peripherals(m->block_parent,vec);
    m->vec = vec;
    std::map<std::string,std::vector<Block*> >::iterator it = vec.begin();
    while(it != vec.end()){
        //std::cout << it->first << " --- ";
        //for(int i=0;i<it->second.size();i++)
            //std::cout << it->second[i]->menu->component[0]->name << " ";
        //std::cout << std::endl;
        //std::cout << "size=" << m->component.size() << std::endl;
        for(int i=0;i<18;i+=3){
            //std::cout << m->component[i+1]->name << std::endl;
            if(it->first == m->component[i+1]->name){
                std::stringstream ss;
                for(int j=0;j<it->second.size();j++)
                    ss << it->second[j]->menu->component[0]->name << "; ";
                m->component[i+2]->name = ss.str();
            }
        }
        ++it;
    }
    {
    std::stringstream ss;
    ss << asteroid_wx;
    m->block_parent->menu->component[19]->name = ss.str(); 
    }
    {
    std::stringstream ss;
    ss << asteroid_wy;
    m->block_parent->menu->component[20]->name = ss.str(); 
    }
    {
    std::stringstream ss;
    ss << asteroid_wz;
    m->block_parent->menu->component[21]->name = ss.str(); 
    }
    }
}

void calculate_relative_center_of_mass(Block* b,float& x,float& y,float& z,float& m){
    int color = ++start_color;
    std::stack<Block*> Q;
    Block* t;
    Q.push(b);
    double x_c = 0.0;
    double y_c = 0.0;
    double z_c = 0.0;
    double n = 1.0;
    double mass;
    while(Q.size() > 0){
        t = Q.top();
        Q.pop();
        {
            t->color = color;
            mass = 1.0;
            x_c += (mass * t->x - x_c) / n;
            y_c += (mass * t->y - y_c) / n;
            z_c += (mass * t->z - z_c) / n;
            n += mass;
            if(t->x+1<100)
            if(data         [t->x+1][t->y][t->z]!=NULL)
            if(data         [t->x+1][t->y][t->z]->type > 0)
            if(data         [t->x+1][t->y][t->z]->color != color){
            data            [t->x+1][t->y][t->z]->color = color;
            Q.push(data     [t->x+1][t->y][t->z]);
            }
            if(t->x-1>=0)
            if(data         [t->x-1][t->y][t->z]!=NULL)
            if(data         [t->x-1][t->y][t->z]->type > 0)
            if(data         [t->x-1][t->y][t->z]->color != color){
            data            [t->x-1][t->y][t->z]->color = color;
            Q.push(data     [t->x-1][t->y][t->z]);
            }
            if(t->y+1<100)
            if(data         [t->x][t->y+1][t->z]!=NULL)
            if(data         [t->x][t->y+1][t->z]->type > 0)
            if(data         [t->x][t->y+1][t->z]->color != color){
            data            [t->x][t->y+1][t->z]->color = color;
            Q.push(data     [t->x][t->y+1][t->z]);
            }
            if(t->y-1>=0)
            if(data         [t->x][t->y-1][t->z]!=NULL)
            if(data         [t->x][t->y-1][t->z]->type > 0)
            if(data         [t->x][t->y-1][t->z]->color != color){
            data            [t->x][t->y-1][t->z]->color = color;
            Q.push(data     [t->x][t->y-1][t->z]);
            }
            if(t->z+1<100)
            if(data         [t->x][t->y][t->z+1]!=NULL)
            if(data         [t->x][t->y][t->z+1]->type > 0)
            if(data         [t->x][t->y][t->z+1]->color != color){
            data            [t->x][t->y][t->z+1]->color = color;
            Q.push(data     [t->x][t->y][t->z+1]);
            }
            if(t->z-1>=0)
            if(data         [t->x][t->y][t->z-1]!=NULL)
            if(data         [t->x][t->y][t->z-1]->type > 0)
            if(data         [t->x][t->y][t->z-1]->color != color){
            data            [t->x][t->y][t->z-1]->color = color;
            Q.push(data     [t->x][t->y][t->z-1]);
            }
        }
    }
    x = x_c-50.0;
    y = y_c-50.0;
    z = z_c-50.0;
    m = n;
}

void apply_force(Block* b,float direction_x,float direction_y,float direction_z,float& tx,float& ty,float& tz,float& fx,float& fy,float& fz){ // direction defined in local coordinate system

    float x_c;
    float y_c;
    float z_c;

    float m;

    calculate_relative_center_of_mass(b,x_c,y_c,z_c,m);

    double mass = m;

    double x_r = b->x-50.0-x_c;
    double y_r = b->y-50.0-y_c;
    double z_r = b->z-50.0-z_c;

    double torque_x = y_r * direction_z - z_r * direction_y;
    double torque_y = z_r * direction_x - x_r * direction_z;
    double torque_z = x_r * direction_y - y_r * direction_x;

    tx = torque_x / mass;
    ty = torque_y / mass;
    tz = torque_z / mass;

    double r = sqrt(x_r*x_r + y_r*y_r + z_r*z_r);
    if(r > 1e-5){
        x_r /= r;
        y_r /= r;
        z_r /= r;
        double magnitude = x_r * direction_x + y_r * direction_y + z_r * direction_z;
        double force_x = x_r * magnitude;
        double force_y = y_r * magnitude;
        double force_z = z_r * magnitude;
        fx = force_x / mass;
        fy = force_y / mass;
        fz = force_z / mass;
    }else{
        fx = 0.0;
        fy = 0.0;
        fz = 0.0;
    }

}

std::vector<Block*> blocks;
std::vector<Block*> stars;

Block* add_block(int i,int j,int k,unsigned int type) 
{
    Block* b = new Block(i,j,k,type);
    if(!data[i][j][k]){
        data[i][j][k] = b;
        blocks.push_back(b);
    }
    return b;
}

void delete_block(int i,int j,int k) 
{
    std::vector<Block*>::iterator it = std::find(blocks.begin(),blocks.end(),data[i][j][k]);
    blocks.erase(it);
    delete data[i][j][k];
    data[i][j][k] = NULL;
    
}

void init_blocks()
{

    int n = 100;
    data = new Block *** [n];
    for(int i=0;i<n;i++) {
        data[i] = new Block ** [n];
        for(int j=0;j<n;j++) {
            data[i][j] = new Block * [n];
            for(int k=0;k<n;k++) {
                data[i][j][k] = NULL;
            }
        }
    }

    blocks.clear();

    

}



//int _WIDTH  = 1920;
//int _HEIGHT = 1020;

int _WIDTH  = 800;
int _HEIGHT = 600;


int WIDTH = 800;
int HEIGHT = 600;

ScreenRecorder screen_recorder(WIDTH,HEIGHT);

bool MINE     = false;
bool MENU     = false;
bool PLACE    = false;
bool SHIFT    = false;

bool FORWARD  = false
   , BACKWARD = false
   , LEFT     = false
   , RIGHT    = false
   , UP       = false
   , DOWN     = false
   ;

bool CONTROL_FORWARD  = false
   , CONTROL_BACKWARD = false
   , CONTROL_LEFT     = false
   , CONTROL_RIGHT    = false
   , CONTROL_UP       = false
   , CONTROL_DOWN     = false
   ;

bool RECORD = false;

//GLfloat xRotated  = 0
//      , yRotated  = 0
//      , zRotated  = 0
//      ;

inline bool point_comparator_directional(Block* a,Block* b){

    double xa = (xPosition-1.5*cameraRotation[2]+0.0*mouse_delta_y*cameraRotation[1]+0.0*mouse_delta_x*cameraRotation[0])  - ((a->x-50)*asteroid_m[0]+(a->y-50)*asteroid_m[4]+(a->z-50)*asteroid_m[8]);
    double ya = (yPosition-1.5*cameraRotation[6]+0.0*mouse_delta_y*cameraRotation[5]+0.0*mouse_delta_x*cameraRotation[4])  - ((a->x-50)*asteroid_m[1]+(a->y-50)*asteroid_m[5]+(a->z-50)*asteroid_m[9]);
    double za = (zPosition-1.5*cameraRotation[10]+0.0*mouse_delta_y*cameraRotation[9]+0.0*mouse_delta_x*cameraRotation[8]) - ((a->x-50)*asteroid_m[2]+(a->y-50)*asteroid_m[6]+(a->z-50)*asteroid_m[10]);
    double ra = xa*xa+ya*ya+za*za;

    double xb = (xPosition-1.5*cameraRotation[2]+0.0*mouse_delta_y*cameraRotation[1]+0.0*mouse_delta_x*cameraRotation[0])  - ((b->x-50)*asteroid_m[0]+(b->y-50)*asteroid_m[4]+(b->z-50)*asteroid_m[8]);
    double yb = (yPosition-1.5*cameraRotation[6]+0.0*mouse_delta_y*cameraRotation[5]+0.0*mouse_delta_x*cameraRotation[4])  - ((b->x-50)*asteroid_m[1]+(b->y-50)*asteroid_m[5]+(b->z-50)*asteroid_m[9]);
    double zb = (zPosition-1.5*cameraRotation[10]+0.0*mouse_delta_y*cameraRotation[9]+0.0*mouse_delta_x*cameraRotation[8]) - ((b->x-50)*asteroid_m[2]+(b->y-50)*asteroid_m[6]+(b->z-50)*asteroid_m[10]);
    double rb = xb*xb+yb*yb+zb*zb;

    return ra < rb;

}



inline bool point_comparator(Block* a,Block* b){

    double xa = (xPosition) - ((a->x-50)*asteroid_m[0]+(a->y-50)*asteroid_m[4]+(a->z-50)*asteroid_m[8]);
    double ya = (yPosition) - ((a->x-50)*asteroid_m[1]+(a->y-50)*asteroid_m[5]+(a->z-50)*asteroid_m[9]);
    double za = (zPosition) - ((a->x-50)*asteroid_m[2]+(a->y-50)*asteroid_m[6]+(a->z-50)*asteroid_m[10]);
    double ra = xa*xa+ya*ya+za*za;

    double xb = (xPosition) - ((b->x-50)*asteroid_m[0]+(b->y-50)*asteroid_m[4]+(b->z-50)*asteroid_m[8]);
    double yb = (yPosition) - ((b->x-50)*asteroid_m[1]+(b->y-50)*asteroid_m[5]+(b->z-50)*asteroid_m[9]);
    double zb = (zPosition) - ((b->x-50)*asteroid_m[2]+(b->y-50)*asteroid_m[6]+(b->z-50)*asteroid_m[10]);
    double rb = xb*xb+yb*yb+zb*zb;

    return ra < rb;

}

int mouse_x;
int mouse_y;
double mouse_r;

float _curr_time = 0;
int _curr_index = 0;
int _final_index = 0;

float _mining_curr_time = 0;
int _mining_curr_index = 0;
int _mining_final_index = 0;

float _resources_curr_time = 0;
int _resources_curr_index = 0;
int _resources_final_index = 0;

class Projectile;

class Enemy
{
    public:

    bool ranged;

    bool live;

    float x,y,z;

    float vx,vy,vz;

    float ax,ay,az;

    float attack_range;

    float protect_range;

    float maximum_range;

    float medium_range;

    float minimum_range;

    float health;

    float damage;

    float reload_delay;

    float acceleration;

    float friction_coefficient;

    Enemy(float _x,float _y,float _z)
    {
        x = _x;
        y = _y;
        z = _z;
        live = true;
    }

    void handle_collisions(std::vector<Enemy*>& objs)
    {

        for(int i=0;i<objs.size();i++)
        {
            double dx = objs[i]->x - x;
            double dy = objs[i]->y - y;
            double dz = objs[i]->z - z;
            double d  = sqrt(dx*dx+dy*dy+dz*dz);
            if( d > 1e-5 && d < 5 )
            {
                ax -= acceleration*dx/d;
                ay -= acceleration*dy/d;
                az -= acceleration*dz/d;
            }
        }

        int X,Y,Z;

        X = (int)((x+.5)*asteroid_m[0] + (y+.5)*asteroid_m[1] + (z+.5)*asteroid_m[2] )+50;
        Y = (int)((x+.5)*asteroid_m[4] + (y+.5)*asteroid_m[5] + (z+.5)*asteroid_m[6] )+50;
        Z = (int)((x+.5)*asteroid_m[8] + (y+.5)*asteroid_m[9] + (z+.5)*asteroid_m[10])+50;

        if( X >= 2
         && X <  97
         && Y >= 2
         && Y <  97
         && Z >= 2
         && Z <  97
        )
        {

            std::vector<Block*> points;
            for(int i=-2;i<=2;i++)
                for(int j=-2;j<=2;j++)
                    for(int k=-2;k<=2;k++){
                        Block* p = data[(int)X+i][(int)Y+j][(int)Z+k];
                        if(p != NULL)
                            if(p->type > 0)
                                points.push_back(p);
                    }

            std::sort(points.begin(),points.end(),point_comparator);

            double surface_vx = -(yPosition*asteroid_wz - zPosition*asteroid_wy);
            double surface_vy = -(zPosition*asteroid_wx - xPosition*asteroid_wz);
            double surface_vz = -(xPosition*asteroid_wy - yPosition*asteroid_wx);

            for(int i=0;i<points.size();i++)
                if(points[i] != NULL)
                    if(points[i]->type > 0){
                        double _x = x - ((points[i]->x-50)*asteroid_m[0]+(points[i]->y-50)*asteroid_m[4]+(points[i]->z-50)*asteroid_m[8]);
                        double _y = y - ((points[i]->x-50)*asteroid_m[1]+(points[i]->y-50)*asteroid_m[5]+(points[i]->z-50)*asteroid_m[9]);
                        double _z = z - ((points[i]->x-50)*asteroid_m[2]+(points[i]->y-50)*asteroid_m[6]+(points[i]->z-50)*asteroid_m[10]);
                        double r = sqrt(_x*_x+_y*_y+_z*_z);
                        if(r > 1e-4){
                            _x /= r;
                            _y /= r;
                            _z /= r;
                        }
                        if(r < 1.35){
                            x += (1.35-r)*_x;
                            y += (1.35-r)*_y;
                            z += (1.35-r)*_z;
 
                            vx = .98*(vx-surface_vx) + surface_vx;
                            vy = .98*(vy-surface_vy) + surface_vy;
                            vz = .98*(vz-surface_vz) + surface_vz;
 
                        }

                    }
        }

    }

    float direction_to_attack_target_x;
    float direction_to_attack_target_y;
    float direction_to_attack_target_z;
    float dist_to_attack_target; 
 
    float direction_to_protect_target_x;
    float direction_to_protect_target_y;
    float direction_to_protect_target_z;
    float dist_to_protect_target; 
 

    void update( float attack_target_x
               , float attack_target_y
               , float attack_target_z
               , float protect_target_x
               , float protect_target_y
               , float protect_target_z
               , std::vector<Enemy*>& objs
               )
    {

        if(health <= 0)
            live = false;

        handle_collisions(objs);

        // update physical quantities
        x += vx;
        y += vy;
        z += vz;

        ax -= friction_coefficient*vx;
        ay -= friction_coefficient*vy;
        az -= friction_coefficient*vz;

        vx += ax;
        vy += ay;
        vz += az;


        // calculate attack target direction
        direction_to_attack_target_x = attack_target_x - x;
        direction_to_attack_target_y = attack_target_y - y;
        direction_to_attack_target_z = attack_target_z - z;
        dist_to_attack_target = sqrt( direction_to_attack_target_x*direction_to_attack_target_x 
                                    + direction_to_attack_target_y*direction_to_attack_target_y
                                    + direction_to_attack_target_z*direction_to_attack_target_z
                                    );
        if(dist_to_attack_target > 1.0)
        {
            direction_to_attack_target_x /= dist_to_attack_target;
            direction_to_attack_target_y /= dist_to_attack_target;
            direction_to_attack_target_z /= dist_to_attack_target;
        }

        // calculate protect target direction
        direction_to_protect_target_x = protect_target_x - x;
        direction_to_protect_target_y = protect_target_y - y;
        direction_to_protect_target_z = protect_target_z - z;
        dist_to_protect_target = sqrt( direction_to_protect_target_x*direction_to_protect_target_x
                                     + direction_to_protect_target_y*direction_to_protect_target_y
                                     + direction_to_protect_target_z*direction_to_protect_target_z
                                     );
        if(dist_to_protect_target > 1.0)
        {
            direction_to_protect_target_x /= dist_to_protect_target;
            direction_to_protect_target_y /= dist_to_protect_target;
            direction_to_protect_target_z /= dist_to_protect_target;
        }

        // calculate velocity direction
        float direction_velocity_x = vx;
        float direction_velocity_y = vy;
        float direction_velocity_z = vz;
        float dist_velocity = sqrt(vx*vx + vy*vy + vz*vz);
        if(dist_velocity > 1.0)
        {
            direction_velocity_x /= dist_velocity;
            direction_velocity_y /= dist_velocity;
            direction_velocity_z /= dist_velocity;
        }

        // if dist_to_attack_target > maximum_range && dist_to_protect_target < protect_range
        // move straight
        {
            // do nothing
            // move straight
            ax = acceleration*direction_velocity_x;
            ay = acceleration*direction_velocity_y;
            az = acceleration*direction_velocity_z;
        }


        // if dist_to_attack_target > maximum_range && dist_to_protect_target > protect_range
        // move towards protect_target
        if( dist_to_attack_target > maximum_range && dist_to_protect_target > protect_range )
        {
            ax = acceleration*direction_to_protect_target_x;
            ay = acceleration*direction_to_protect_target_y;
            az = acceleration*direction_to_protect_target_z;
        }


        // if dist_to_attack_target < maximum_range
        // move towards attack_target
        if( dist_to_attack_target < maximum_range )
        {
            ax = acceleration*direction_to_attack_target_x;
            ay = acceleration*direction_to_attack_target_y;
            az = acceleration*direction_to_attack_target_z;
        }

        // if dist_to_attack_target < attack_range
        // start attacking
        if( dist_to_attack_target < attack_range )
        {
            // attacking
        }

        // if dist_to_attack_target < minimum_range
        // move away from attack_target
        if( dist_to_attack_target < medium_range )
        {
            ax = 0;
            ay = 0;
            az = 0;
        }

        // if dist_to_attack_target < minimum_range
        // move away from attack_target
        if( dist_to_attack_target < minimum_range )
        {
            ax = -acceleration*direction_to_attack_target_x;
            ay = -acceleration*direction_to_attack_target_y;
            az = -acceleration*direction_to_attack_target_z;
        }

    }

    float theta;
    GLfloat enemy_m[16];
    float forward_x  , forward_y   , forward_z  , forward_r;
    float top_x      , top_y       , top_z      , top_r    ;
    float right_x    , right_y     , right_z    , right_r  ;


    virtual void draw() = 0;
    

};



std::vector<Projectile*> projectile;
std::vector<Projectile*> mining_projectile;
std::vector<Projectile*> resource_projectile;
std::vector<Enemy*> enemy;

class Projectile
{
    public:

    float x,y,z;

    float vx,vy,vz;

    float rx,ry,rz;

    bool live;

    double block_damage;

    double player_damage;

    bool produce_resource;

    bool is_resource;

    int resource_type;

    int side;

    float start_time;

    float end_time;

    Projectile()
    {
        block_damage = 0;
        player_damage = 0;
        produce_resource = false;
        is_resource = false;
        live = false;
        start_time = 0;
        end_time   = 0;
    }

    Projectile( float _x
              , float _y
              , float _z
              , float _vx
              , float _vy
              , float _vz
              )
              : x  ( _x  )
              , y  ( _y  )
              , z  ( _z  )
              , vx ( _vx )
              , vy ( _vy )
              , vz ( _vz )
    {

        block_damage = 0;
        player_damage = 0;
        produce_resource = false;
        is_resource = false;
        live = false;

    }

    void collision()
    {

        if(player_damage > 0){
            if( sqrt( pow( xPosition-x, 2 )
                    + pow( yPosition-y, 2 )
                    + pow( zPosition-z, 2 )
                    ) < 0.25
            )
            {
                
                player->life -= player_damage;
                live = false;
                return;
            }
            for(int i=0;i<enemy.size();i++)
            {
                if( sqrt( pow( enemy[i]->x-x, 2 )
                        + pow( enemy[i]->y-y, 2 )
                        + pow( enemy[i]->z-z, 2 )
                        ) < 0.25
                )
                {
                    
                    enemy[i]->health -= player_damage;
                    live = false;
                    return;
                }
            }
        }

        if(is_resource)
        if( sqrt( pow( xPosition-x, 2 )
                + pow( yPosition-y, 2 )
                + pow( zPosition-z, 2 )
                ) < 3.25
        )
        {
            
            if(player->tool.insert_item(resource_type) == 0){
                live = false;
                return;
            }
        }



        int X,Y,Z;

        X = (int)((x+.5)*asteroid_m[0] + (y+.5)*asteroid_m[1] + (z+.5)*asteroid_m[2] )+50;
        Y = (int)((x+.5)*asteroid_m[4] + (y+.5)*asteroid_m[5] + (z+.5)*asteroid_m[6] )+50;
        Z = (int)((x+.5)*asteroid_m[8] + (y+.5)*asteroid_m[9] + (z+.5)*asteroid_m[10])+50;

        if( X >= 2
         && X <  97
         && Y >= 2
         && Y <  97
         && Z >= 2
         && Z <  97
        )
        {

            std::vector<Block*> points;
            for(int i=-2;i<=2;i++)
                for(int j=-2;j<=2;j++)
                    for(int k=-2;k<=2;k++){
                        Block* p = data[(int)X+i][(int)Y+j][(int)Z+k];
                        if(p != NULL)
                            if(p->type > 0)
                                points.push_back(p);
                    }

            std::sort(points.begin(),points.end(),point_comparator);

            double surface_vx = -(yPosition*asteroid_wz - zPosition*asteroid_wy);
            double surface_vy = -(zPosition*asteroid_wx - xPosition*asteroid_wz);
            double surface_vz = -(xPosition*asteroid_wy - yPosition*asteroid_wx);

            for(int i=0;i<points.size();i++)
                if(points[i] != NULL)
                    if(points[i]->type > 0){
                        double _x = x - ((points[i]->x-50)*asteroid_m[0]+(points[i]->y-50)*asteroid_m[4]+(points[i]->z-50)*asteroid_m[8]);
                        double _y = y - ((points[i]->x-50)*asteroid_m[1]+(points[i]->y-50)*asteroid_m[5]+(points[i]->z-50)*asteroid_m[9]);
                        double _z = z - ((points[i]->x-50)*asteroid_m[2]+(points[i]->y-50)*asteroid_m[6]+(points[i]->z-50)*asteroid_m[10]);
                        double r = sqrt(_x*_x+_y*_y+_z*_z);
                        if(r > 1e-4){
                            _x /= r;
                            _y /= r;
                            _z /= r;
                        }

                        if( r < .75 || (is_resource && r < 1.35) ){

                            if(is_resource == false){
                                live = false;
                                if( block_damage > points[i]->armor)
                                {
                                    points[i]->health -= block_damage - points[i]->armor;
                                    if(points[i]->health < 0){

                                        if(produce_resource) 
                                        {
                                            resource_projectile[_resources_final_index]->timer = 0;
                                            resource_projectile[_resources_final_index]->resource_type = points[i]->type;
                                            resource_projectile[_resources_final_index]->x = x - _x;
                                            resource_projectile[_resources_final_index]->y = y - _y;
                                            resource_projectile[_resources_final_index]->z = z - _z;

                                            resource_projectile[_resources_final_index]->vx = .0001*(1.0-(rand()%1000)/500.0);
                                            resource_projectile[_resources_final_index]->vy = .0001*(1.0-(rand()%1000)/500.0);
                                            resource_projectile[_resources_final_index]->vz = .0001*(1.0-(rand()%1000)/500.0);

                                            resource_projectile[_resources_final_index]->rx = .01*(1.0-(rand()%1000)/500.0);
                                            resource_projectile[_resources_final_index]->ry = .01*(1.0-(rand()%1000)/500.0);
                                            resource_projectile[_resources_final_index]->rz = .01*(1.0-(rand()%1000)/500.0);

                                            resource_projectile[_resources_final_index]->start_time = _resources_curr_time;
                                            resource_projectile[_resources_final_index]->end_time   = _resources_curr_time+10;
                                            resource_projectile[_resources_final_index]->live       = true;
                                            _resources_final_index++;
                                            if(_resources_final_index >= resource_projectile.size())
                                                _resources_final_index = 0;
                                        }
    

                                        if(selection == points[i])
                                            selection = NULL;
                                        mesh.RemoveBox( data[points[i]->x][points[i]->y][points[i]->z] );
                                        delete_block( points[i]->x
                                                    , points[i]->y
                                                    , points[i]->z
                                                    );
                                        update_scene = true;
                                    }
                                }
                            }else
                            {

                                x += (1.35-r)*_x;
                                y += (1.35-r)*_y;
                                z += (1.35-r)*_z;
 
                                vx = .98*(vx-surface_vx) + surface_vx;
                                vy = .98*(vy-surface_vy) + surface_vy;
                                vz = .98*(vz-surface_vz) + surface_vz;

                            }

                        }

                    }
        }


    }

    void update()
    {
        if(live)
        {
            x += vx;
            y += vy;
            z += vz;
            collision();
        }
    }

    GLfloat enemy_m[16];
    int timer;

    virtual void draw() = 0;

};


class WeaponProjectile:public Projectile
{
    public:

    WeaponProjectile()
    : Projectile()
    {

        block_damage = 5;
        player_damage = 1;
        produce_resource = false;

    }

    WeaponProjectile( float _x
                   , float _y
                   , float _z
                   , float _vx
                   , float _vy
                   , float _vz
                   )
    : Projectile( _x
                , _y
                , _z
                , _vx
                , _vy
                , _vz
                )
    {
        block_damage = 5;
        player_damage = 1;
        produce_resource = false;
    }

    void draw()
    {
        if(live)
        {
            glPushMatrix();
            glTranslatef(x,y,z);
            if( timer == 0 )
            {
                ++timer;
                float forward_x  , forward_y   , forward_z  , forward_r;
                float top_x   = 0, top_y   =  1, top_z   = 0, top_r    ;
                float right_x = 0, right_y =  1, right_z = 0, right_r  ;
                forward_x = vx;
                forward_y = vy;
                forward_z = vz;
                forward_r = sqrt(forward_x*forward_x + forward_y*forward_y + forward_z*forward_z);
                right_x  = top_y   * forward_z - top_z   * forward_y;
                right_y  = top_z   * forward_x - top_x   * forward_z;
                right_z  = top_x   * forward_y - top_y   * forward_x;
                right_r  = sqrt(right_x*right_x + right_y*right_y + right_z*right_z);
                right_x /= right_r;
                right_y /= right_r;
                right_z /= right_r;
                top_x    = right_y * forward_z - right_z * forward_y;
                top_y    = right_z * forward_x - right_x * forward_z;
                top_z    = right_x * forward_y - right_y * forward_x;
                top_r    = sqrt(top_x*top_x + top_y*top_y + top_z*top_z);
                top_x   /= top_r;
                top_y   /= top_r;
                top_z   /= top_r;
                enemy_m[0]  = .1*right_x   ; enemy_m[1]  = .1*right_y   ; enemy_m[2]   = .1*right_z   ; enemy_m[3]  = 0 ;
                enemy_m[4]  = .1*top_x     ; enemy_m[5]  = .1*top_y     ; enemy_m[6]   = .1*top_z     ; enemy_m[7]  = 0 ;
                enemy_m[8]  = 2 *forward_x ; enemy_m[9]  = 2 *forward_y ; enemy_m[10]  = 2 *forward_z ; enemy_m[11] = 0 ;
                enemy_m[12] = 0            ; enemy_m[13] = 0            ; enemy_m[14]  = 0            ; enemy_m[15] = 1 ;
            }
            glMultMatrixf(enemy_m);

            glBindTexture( GL_TEXTURE_2D, texture[1] );
            glCallList(projectile_sphere);
            //std::cout << x << ":" << xPosition << " ";
            //std::cout << y << ":" << yPosition << " ";
            //std::cout << z << ":" << zPosition << " ";
            //std::cout << std::endl;
            glPopMatrix();
        }
    }


};

class MiningProjectile:public Projectile
{
    public:

    MiningProjectile()
    : Projectile()
    {

        block_damage = 7;
        player_damage = 0;
        produce_resource = true;

    }

    MiningProjectile( float _x
                    , float _y
                    , float _z
                    , float _vx
                    , float _vy
                    , float _vz
                    )
    : Projectile( _x
                , _y
                , _z
                , _vx
                , _vy
                , _vz
                )
    {
        block_damage = 10;
        player_damage = 0;
        produce_resource = true;
    }

    void draw()
    {
    }


};

class ResourceProjectile:public Projectile
{
    public:

    ResourceProjectile()
    : Projectile()
    {

        block_damage = 0;
        resource_type = 0;
        is_resource = true;
        player_damage = 0;
        produce_resource = true;
        angle = 0;

    }

    ResourceProjectile( float _x
                      , float _y
                      , float _z
                      , float _vx
                      , float _vy
                      , float _vz
                      )
    : Projectile( _x
                , _y
                , _z
                , _vx
                , _vy
                , _vz
                )
    {
        block_damage = 0;
        resource_type = 0;
        is_resource = true;
        player_damage = 0;
        produce_resource = true;
        angle = 0;
    }

    float angle;

    void draw()
    {
        if(live)
        {
            glPushMatrix();
            glTranslatef(x,y,z);

            glRotatef(angle,rx,ry,rz);
            angle += 5;

            glBindTexture( GL_TEXTURE_2D, texture[0] );
            float su = .125*((int)resource_type%8);
            float eu = su+.125;

            float sv = .125*((int)resource_type/8);
            float ev = sv+.125;

            float fact = 0.1;

            glBegin(GL_QUADS);

            glTexCoord2f(su,sv);            glVertex3f( fact,  fact, -fact);
            glTexCoord2f(su,ev);            glVertex3f(-fact,  fact, -fact);
            glTexCoord2f(eu,ev);            glVertex3f(-fact,  fact,  fact);
            glTexCoord2f(eu,sv);            glVertex3f( fact,  fact,  fact);

            glTexCoord2f(su,sv);            glVertex3f( fact, -fact,  fact);
            glTexCoord2f(su,ev);            glVertex3f(-fact, -fact,  fact);
            glTexCoord2f(eu,ev);            glVertex3f(-fact, -fact, -fact);
            glTexCoord2f(eu,sv);            glVertex3f( fact, -fact, -fact);

            glTexCoord2f(su,sv);            glVertex3f( fact,  fact,  fact);
            glTexCoord2f(su,ev);            glVertex3f(-fact,  fact,  fact);
            glTexCoord2f(eu,ev);            glVertex3f(-fact, -fact,  fact);
            glTexCoord2f(eu,sv);            glVertex3f( fact, -fact,  fact);

            glTexCoord2f(su,sv);            glVertex3f( fact, -fact, -fact);
            glTexCoord2f(su,ev);            glVertex3f(-fact, -fact, -fact);
            glTexCoord2f(eu,ev);            glVertex3f(-fact,  fact, -fact);
            glTexCoord2f(eu,sv);            glVertex3f( fact,  fact, -fact);

            glTexCoord2f(su,sv);            glVertex3f(-fact,  fact,  fact);
            glTexCoord2f(su,ev);            glVertex3f(-fact,  fact, -fact);
            glTexCoord2f(eu,ev);            glVertex3f(-fact, -fact, -fact);
            glTexCoord2f(eu,sv);            glVertex3f(-fact, -fact,  fact);

            glTexCoord2f(su,sv);            glVertex3f( fact,  fact, -fact);
            glTexCoord2f(su,ev);            glVertex3f( fact,  fact,  fact);
            glTexCoord2f(eu,ev);            glVertex3f( fact, -fact,  fact);
            glTexCoord2f(eu,sv);            glVertex3f( fact, -fact, -fact);

            glEnd();  

            //glCallList(projectile_sphere);
            //std::cout << x << ":" << xPosition << " ";
            //std::cout << y << ":" << yPosition << " ";
            //std::cout << z << ":" << zPosition << " ";
            //std::cout << std::endl;
            glPopMatrix();
        }
    }


};


void update(std::vector<Projectile*>& p_projectile,float curr_time,int & curr_index,int final_index)
{
    for(int i=curr_index;;++i)
    {
        if(i>=p_projectile.size())
        {
            i=0;
        }
        if(i == final_index)
        {
            break;
        }
        if(curr_time>=p_projectile[i]->start_time && curr_time<p_projectile[i]->end_time)
        {
            p_projectile[i]->update();
        }
    }

}

void draw  (std::vector<Projectile*>& p_projectile,float curr_time,int & curr_index,int final_index)
{
    bool found_curr = false;
    for(int i=curr_index;;++i)
    {
        if(i>=p_projectile.size())
        {
            i=0;
        }
        if(i == final_index)
        {
            break;
        }
        if(curr_time>=p_projectile[i]->start_time && curr_time<p_projectile[i]->end_time)
        {
            if(!found_curr)
                curr_index = i;
            found_curr = true;
            p_projectile[i]->draw();
        }
    }
}

class FighterEnemy : public Enemy
{
    public:

    FighterEnemy(float _x,float _y,float _z)
    : Enemy( _x, _y, _z )
    {
        ranged = true;
        theta = 0;
        x = _x;
        y = _y;
        z = _z;
        vx = 0;
        vy = 0;
        vz = 0;
        ax = 0;
        ay = 0;
        az = 0;
        attack_range = 40;
        protect_range = 30;
        minimum_range = 2;//20;
        medium_range = 3;//30;
        maximum_range = 5000;
        health = 100;
        damage = 1;
        reload_delay = 5;
        acceleration = .0004;
        friction_coefficient = .005;
    }

    void draw()
    {
        glPushMatrix();
        // x_vector : x coord - enemy_m[0]
        // x_vector : y coord - enemy_m[1]
        // x_vector : z coord - enemy_m[2]
        // y_vector : x coord - enemy_m[4]
        // y_vector : y coord - enemy_m[5]
        // y_vector : z coord - enemy_m[6]
        // z_vector : x coord - enemy_m[8]
        // z_vector : y coord - enemy_m[9]
        // z_vector : z coord - enemy_m[10]
        // x - enemy_m[12]
        // y - enemy_m[13]
        // z - enemy_m[14]
        static int timer = 0;
        timer++;
        glTranslatef(x,y,z);
        if( timer > 0 )
        {
            timer = 0;
            top_x   = 0; top_y   =  -1; top_z   = 0;
            right_x = 0; right_y =  -1; right_z = 0;
            if( dist_to_attack_target < maximum_range )
            {
                forward_x = direction_to_attack_target_x;
                forward_y = direction_to_attack_target_y;
                forward_z = direction_to_attack_target_z;
            }
            else
            {
                forward_x = direction_to_protect_target_x;
                forward_y = direction_to_protect_target_y;
                forward_z = direction_to_protect_target_z;
            }
            right_x  = top_y   * forward_z - top_z   * forward_y;
            right_y  = top_z   * forward_x - top_x   * forward_z;
            right_z  = top_x   * forward_y - top_y   * forward_x;
            right_r  = sqrt(right_x*right_x + right_y*right_y + right_z*right_z);
            right_x /= right_r;
            right_y /= right_r;
            right_z /= right_r;
            top_x    = right_y * forward_z - right_z * forward_y;
            top_y    = right_z * forward_x - right_x * forward_z;
            top_z    = right_x * forward_y - right_y * forward_x;
            top_r    = sqrt(top_x*top_x + top_y*top_y + top_z*top_z);
            top_x   /= top_r;
            top_y   /= top_r;
            top_z   /= top_r;

            enemy_m[0]  += ( right_x   - enemy_m[0] )*.01; enemy_m[1]  += ( right_y   - enemy_m[1] )*.01; enemy_m[2]   += ( right_z   - enemy_m[2] )*.01; enemy_m[3]  = 0;
            enemy_m[4]  += ( top_x     - enemy_m[4] )*.01; enemy_m[5]  += ( top_y     - enemy_m[5] )*.01; enemy_m[6]   += ( top_z     - enemy_m[6] )*.01; enemy_m[7]  = 0;
            enemy_m[8]  += (-forward_x - enemy_m[8] )*.01; enemy_m[9]  += (-forward_y - enemy_m[9] )*.01; enemy_m[10]  += (-forward_z - enemy_m[10])*.01; enemy_m[11] = 0;

            //enemy_m[8]  += (-right_x   - enemy_m[8] )*.01; enemy_m[9]  += (-right_y   - enemy_m[9] )*.01; enemy_m[10] += (-right_z   - enemy_m[10])*.01; enemy_m[11]  = 0;
            //enemy_m[4]  += ( top_x     - enemy_m[4] )*.01; enemy_m[5]  += ( top_y     - enemy_m[5] )*.01; enemy_m[6]  += ( top_z     - enemy_m[6] )*.01; enemy_m[7]   = 0;
            //enemy_m[0]  += ( forward_x - enemy_m[0] )*.01; enemy_m[1]  += ( forward_y - enemy_m[1] )*.01; enemy_m[2]  += ( forward_z - enemy_m[2] )*.01; enemy_m[3]   = 0;

            enemy_m[12]  = 0;                              enemy_m[13]  = 0;                              enemy_m[14]   = 0;                              enemy_m[15] = 1;
            forward_r = sqrt(enemy_m[8]*enemy_m[8] + enemy_m[9]*enemy_m[9] + enemy_m[10]*enemy_m[10]);
            enemy_m[8]  /= forward_r;
            enemy_m[9]  /= forward_r;
            enemy_m[10] /= forward_r;
/*
            enemy_m[0]   = enemy_m[5]*enemy_m[10] - enemy_m[6]*enemy_m[9 ];
            enemy_m[1]   = enemy_m[6]*enemy_m[8 ] - enemy_m[4]*enemy_m[10];
            enemy_m[2]   = enemy_m[4]*enemy_m[9 ] - enemy_m[5]*enemy_m[8 ];
*/
            right_r = sqrt(enemy_m[0]*enemy_m[0] + enemy_m[1]*enemy_m[1] + enemy_m[2]*enemy_m[2]);
            enemy_m[0]  /= right_r;
            enemy_m[1]  /= right_r;
            enemy_m[2]  /= right_r;
/*
            enemy_m[3]   = enemy_m[1]*enemy_m[10] - enemy_m[2]*enemy_m[9 ];
            enemy_m[4]   = enemy_m[2]*enemy_m[8 ] - enemy_m[0]*enemy_m[10];
            enemy_m[5]   = enemy_m[0]*enemy_m[9 ] - enemy_m[1]*enemy_m[8 ];
*/
            top_r = sqrt(enemy_m[3]*enemy_m[3] + enemy_m[4]*enemy_m[4] + enemy_m[5]*enemy_m[5]);
            enemy_m[3]  /= top_r;
            enemy_m[4]  /= top_r;
            enemy_m[5]  /= top_r;

        }
        glMultMatrixf(enemy_m);
        glBindTexture( GL_TEXTURE_2D, texture[0] );//MARK
        //glCallList(enemy_sphere);
        DrawMesh(enemy_drone_cmesh);
        //std::cout << x << ":" << xPosition << " ";
        //std::cout << y << ":" << yPosition << " ";
        //std::cout << z << ":" << zPosition << " ";
        //std::cout << std::endl;
        glPopMatrix();
    }

};

class CreatureEnemy : public Enemy
{
    public:

    CreatureEnemy(float _x,float _y,float _z)
    : Enemy( _x, _y, _z )
    {
        ranged = false;
        theta = 0;
        x = _x;
        y = _y;
        z = _z;
        vx = 0;
        vy = 0;
        vz = 0;
        ax = 0;
        ay = 0;
        az = 0;
        attack_range = 40;
        protect_range = 30;
        minimum_range = 0.5;//20;
        medium_range = 0.5;//30;
        maximum_range = 50;
        health = 100;
        damage = 1;
        reload_delay = 5;
        acceleration = .04;
        friction_coefficient = .5;
    }


    void draw()
    {
        glPushMatrix();
        // x_vector : x coord - enemy_m[0]
        // x_vector : y coord - enemy_m[1]
        // x_vector : z coord - enemy_m[2]
        // y_vector : x coord - enemy_m[4]
        // y_vector : y coord - enemy_m[5]
        // y_vector : z coord - enemy_m[6]
        // z_vector : x coord - enemy_m[8]
        // z_vector : y coord - enemy_m[9]
        // z_vector : z coord - enemy_m[10]
        // x - enemy_m[12]
        // y - enemy_m[13]
        // z - enemy_m[14]
        static int timer = 0;
        timer++;
        glTranslatef(x,y,z);
        if( timer > 0 )
        {
            timer = 0;
            top_x   = 0; top_y   =  1; top_z   = 0;
            right_x = 0; right_y =  1; right_z = 0;
            if( dist_to_attack_target < maximum_range )
            {
                forward_x = -direction_to_attack_target_x;
                forward_y = -direction_to_attack_target_y;
                forward_z = -direction_to_attack_target_z;
            }
            else
            {
                forward_x = -direction_to_protect_target_x;
                forward_y = -direction_to_protect_target_y;
                forward_z = -direction_to_protect_target_z;
            }
            right_x  = top_y   * forward_z - top_z   * forward_y;
            right_y  = top_z   * forward_x - top_x   * forward_z;
            right_z  = top_x   * forward_y - top_y   * forward_x;
            right_r  = -sqrt(right_x*right_x + right_y*right_y + right_z*right_z);
            right_x /= right_r;
            right_y /= right_r;
            right_z /= right_r;
            top_x    = right_y * forward_z - right_z * forward_y;
            top_y    = right_z * forward_x - right_x * forward_z;
            top_z    = right_x * forward_y - right_y * forward_x;
            top_r    = sqrt(top_x*top_x + top_y*top_y + top_z*top_z);
            top_x   /= top_r;
            top_y   /= top_r;
            top_z   /= top_r;

            enemy_m[0]  += ( right_x   - enemy_m[0] )*.01; enemy_m[1]  += ( right_y   - enemy_m[1] )*.01; enemy_m[2]   += ( right_z   - enemy_m[2] )*.01; enemy_m[3]  = 0;
            enemy_m[4]  += ( top_x     - enemy_m[4] )*.01; enemy_m[5]  += ( top_y     - enemy_m[5] )*.01; enemy_m[6]   += ( top_z     - enemy_m[6] )*.01; enemy_m[7]  = 0;
            enemy_m[8]  += (-forward_x - enemy_m[8] )*.01; enemy_m[9]  += (-forward_y - enemy_m[9] )*.01; enemy_m[10]  += (-forward_z - enemy_m[10])*.01; enemy_m[11] = 0;

            //enemy_m[8]  += (-right_x   - enemy_m[8] )*.01; enemy_m[9]  += (-right_y   - enemy_m[9] )*.01; enemy_m[10] += (-right_z   - enemy_m[10])*.01; enemy_m[11]  = 0;
            //enemy_m[4]  += ( top_x     - enemy_m[4] )*.01; enemy_m[5]  += ( top_y     - enemy_m[5] )*.01; enemy_m[6]  += ( top_z     - enemy_m[6] )*.01; enemy_m[7]   = 0;
            //enemy_m[0]  += ( forward_x - enemy_m[0] )*.01; enemy_m[1]  += ( forward_y - enemy_m[1] )*.01; enemy_m[2]  += ( forward_z - enemy_m[2] )*.01; enemy_m[3]   = 0;

            enemy_m[12]  = 0;                              enemy_m[13]  = 0;                              enemy_m[14]   = 0;                              enemy_m[15] = 1;
            forward_r = sqrt(enemy_m[8]*enemy_m[8] + enemy_m[9]*enemy_m[9] + enemy_m[10]*enemy_m[10]);
            enemy_m[8]  /= forward_r;
            enemy_m[9]  /= forward_r;
            enemy_m[10] /= forward_r;
/*
            enemy_m[0]   = enemy_m[5]*enemy_m[10] - enemy_m[6]*enemy_m[9 ];
            enemy_m[1]   = enemy_m[6]*enemy_m[8 ] - enemy_m[4]*enemy_m[10];
            enemy_m[2]   = enemy_m[4]*enemy_m[9 ] - enemy_m[5]*enemy_m[8 ];
*/
            right_r = sqrt(enemy_m[0]*enemy_m[0] + enemy_m[1]*enemy_m[1] + enemy_m[2]*enemy_m[2]);
            enemy_m[0]  /= right_r;
            enemy_m[1]  /= right_r;
            enemy_m[2]  /= right_r;
/*
            enemy_m[3]   = enemy_m[1]*enemy_m[10] - enemy_m[2]*enemy_m[9 ];
            enemy_m[4]   = enemy_m[2]*enemy_m[8 ] - enemy_m[0]*enemy_m[10];
            enemy_m[5]   = enemy_m[0]*enemy_m[9 ] - enemy_m[1]*enemy_m[8 ];
*/
            top_r = sqrt(enemy_m[3]*enemy_m[3] + enemy_m[4]*enemy_m[4] + enemy_m[5]*enemy_m[5]);
            enemy_m[3]  /= top_r;
            enemy_m[4]  /= top_r;
            enemy_m[5]  /= top_r;

        }
        glMultMatrixf(enemy_m);
        glBindTexture( GL_TEXTURE_2D, texture[2] );
        //glCallList(enemy_sphere);
        DrawMesh(enemy_creature_cmesh);
        //std::cout << x << ":" << xPosition << " ";
        //std::cout << y << ":" << yPosition << " ";
        //std::cout << z << ":" << zPosition << " ";
        //std::cout << std::endl;
        glPopMatrix();
    }

};


double calculate_intersection_time_between_ray_and_sphere( float sphere_origin_x
                                                         , float sphere_origin_y
                                                         , float sphere_origin_z
                                                         , float sphere_velocity
                                                         , float ray_origin_x
                                                         , float ray_origin_y
                                                         , float ray_origin_z
                                                         , float ray_velocity_x
                                                         , float ray_velocity_y
                                                         , float ray_velocity_z
                                                         )
{
    float A = ray_velocity_x*ray_velocity_x
            + ray_velocity_y*ray_velocity_y
            + ray_velocity_z*ray_velocity_z
            - sphere_velocity*sphere_velocity
            ;
    float dx = ray_origin_x - sphere_origin_x;
    float dy = ray_origin_y - sphere_origin_y;
    float dz = ray_origin_z - sphere_origin_z;
    float B = 2 * ( dx * ray_velocity_x
                  + dy * ray_velocity_y
                  + dz * ray_velocity_z
                  );
    float C = dx*dx + dy*dy + dz*dz;
    float discriminant = B*B - 4*A*C;
    if( discriminant < 0 )
    {
        return 0;
    }
    float time1 = (-B + sqrt(discriminant))/(2*A);
    float time2 = (-B - sqrt(discriminant))/(2*A);
    if( time1 < 0 && time2 < 0 )
        return 0;
    if( time1 < 0 )
        return time2;
    if( time2 < 0 )
        return time1;
    return min(time1,time2);
}



void DrawCube(void)
{

    glMatrixMode(GL_MODELVIEW);
    // clear the drawing buffer.
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();


    // rotation about Z axis
    //glRotatef(zRotated,0.0,0.0,1.0);
    // rotation about X axis
    //glRotatef(xRotated,1.0,0.0,0.0);
    // rotation about Y axis
    //glRotatef(yRotated,0.0,1.0,0.0);

    //rotateMatrixAroundVector( cameraRotation[0], cameraRotation[4], cameraRotation[8]
    //                        , cameraRotation[1], cameraRotation[5], cameraRotation[9]
    //                        , cameraRotation[2], cameraRotation[6], cameraRotation[10]
    //                        , 0, 0.01, 0
    //                        );

    //glGetFloatv(GL_MODELVIEW_MATRIX, cameraRotation);

    //for(int i=0;i<16;i++){
    //    //std::cout << cameraRotation[i] << " ";
    //    if((i+1)%4==0)
    //        //std::cout << std::endl;
    //}
    ////std::cout << std::endl;

    //cameraRotation[12] = -xPosition;
    //cameraRotation[13] = -yPosition;
    //cameraRotation[14] = -zPosition;

    glPushMatrix();

    glLoadMatrixf(cameraRotation);

    glPushMatrix();
    //glScalef(10000.0f,10000.0f,10000.0f);
    glCullFace(GL_BACK);
    glBindTexture( GL_TEXTURE_2D, texture[1] );
    //glCallList(skybox);
    glColor3f(1.0f,1.0f,1.0f);
    glTexCoord2d(0,0);

    glBegin(GL_POINTS);

    for(int i=0;i<stars.size();i++){
        glVertex3f( stars[i]->x
                  , stars[i]->y
                  , stars[i]->z
                  );
    }

    glEnd();

    glPushMatrix();
    glCullFace(GL_FRONT);
    glTranslatef(0,0,0);
    glScalef(25000,25000,25000);
    glBindTexture( GL_TEXTURE_2D, texture[4] );
    //static double angle = 0;
    //angle -= .1;
    //glRotatef(angle,0,0,1);
    glCallList(skybox);
    glCullFace(GL_BACK);
    glPopMatrix();

    glPushMatrix();
    glCullFace(GL_BACK);
    glTranslatef(1500,0,0);
    glScalef(500,500,500);

    glBindTexture( GL_TEXTURE_2D, texture[5] );
    static double angle = 0;
    angle -= .001;
    glRotatef(angle,0,0,1);
    glCallList(skybox);
    glScalef( 1.005
            , 1.005
            , 1.005
            );

    bool contin;
    pthread_mutex_lock(&lock);
    contin = signal_Jupiter_texture;
    pthread_mutex_unlock(&lock);

        glBindTexture( GL_TEXTURE_2D
                     , texture[6]
                     );

    if(signal_Jupiter_texture)
    {
        signal_Jupiter_texture = false;
        glPixelStorei( GL_UNPACK_ALIGNMENT
                     , 1
                     );

        glTexParameteri( GL_TEXTURE_2D
                       , GL_TEXTURE_WRAP_S
                       , GL_REPEAT
                       );
        glTexParameteri( GL_TEXTURE_2D
                       , GL_TEXTURE_WRAP_T
                       , GL_REPEAT
                       );
        glTexParameteri( GL_TEXTURE_2D
                       , GL_TEXTURE_MAG_FILTER
                       , GL_NEAREST
                       );
        glTexParameteri( GL_TEXTURE_2D
                       , GL_TEXTURE_MIN_FILTER
                       , GL_NEAREST
                       );
        pthread_mutex_lock(&lock);
        glTexImage2D( GL_TEXTURE_2D
                    , 0
                    , GL_RGBA
                    , JimageWidth
                    , JimageHeight
                    , 0
                    , GL_RGBA
                    , GL_UNSIGNED_BYTE
                    , Jupiter_cloud_generator.ret_checkImage
                    );
        pthread_mutex_unlock(&lock);
    }

    glRotatef(angle*2.0,0,0,1);
    glEnable(GL_BLEND);
    glCallList(skybox);
    glDisable(GL_BLEND);
    glCullFace(GL_BACK);
    glPopMatrix();


    glPopMatrix();

    for(int i=0;i<enemy.size();i++)
    {
        if(enemy[i]->live)
        enemy[i]->update( xPosition
                        , yPosition
                        , zPosition
                        , xPosition
                        , yPosition
                        , zPosition
                        , enemy
                        );
    }

    static int fire_type=0;
    _curr_time += .01;
    _mining_curr_time += .01;
    _resources_curr_time += .01;
    /*
    if(rand()%10 == 5)
    {
        fire_type++;
        if(fire_type%3==0)
        {
            projectile[_final_index]->x = xPosition - .10*cameraRotation[1];
            projectile[_final_index]->y = yPosition - .10*cameraRotation[5];
            projectile[_final_index]->z = zPosition - .10*cameraRotation[9];
        }else
        if(fire_type%3==1)
        {
            projectile[_final_index]->x = xPosition - .10*cameraRotation[0];
            projectile[_final_index]->y = yPosition - .10*cameraRotation[4];
            projectile[_final_index]->z = zPosition - .10*cameraRotation[8];
        }else
        if(fire_type%3==2)
        {
            projectile[_final_index]->x = xPosition + .10*cameraRotation[0];
            projectile[_final_index]->y = yPosition + .10*cameraRotation[4];
            projectile[_final_index]->z = zPosition + .10*cameraRotation[8];
        }


        projectile[_final_index]->vx = (enemy->x-xPosition);
        projectile[_final_index]->vy = (enemy->y-yPosition);
        projectile[_final_index]->vz = (enemy->z-zPosition);
        float vel = sqrt(projectile[_final_index]->vx*projectile[_final_index]->vx
                        +projectile[_final_index]->vy*projectile[_final_index]->vy
                        +projectile[_final_index]->vz*projectile[_final_index]->vz
                        );
        if(vel > 1e-5)
        {
            projectile[_final_index]->vx/=vel;
            projectile[_final_index]->vy/=vel;
            projectile[_final_index]->vz/=vel;
        }
        projectile[_final_index]->vx*=0.5;
        projectile[_final_index]->vy*=0.5;
        projectile[_final_index]->vz*=0.5;
        projectile[_final_index]->start_time = _curr_time;
        projectile[_final_index]->end_time = _curr_time+5;
        projectile[_final_index]->live = true;
        _final_index++;
        if(_final_index >= projectile.size())
            _final_index = 0;
    }
    */

     
    for(int i=0;i<enemy.size();i++)
    {
        if(enemy[i]->live)
        if(enemy[i]->ranged)
        if(enemy[i]->dist_to_attack_target < enemy[i]->attack_range)
        if(enemy[i]->enemy_m[8]*enemy[i]->forward_x + enemy[i]->enemy_m[9]*enemy[i]->forward_y + enemy[i]->enemy_m[10]*enemy[i]->forward_z < -.9)
        if(rand()%10 == 5)
        {
    
            float intersection_time = calculate_intersection_time_between_ray_and_sphere( enemy[i]->x+enemy[i]->vx
                                                                                        , enemy[i]->y+enemy[i]->vy
                                                                                        , enemy[i]->z+enemy[i]->vz
                                                                                        , 0.5
                                                                                        , xPosition
                                                                                        , yPosition
                                                                                        , zPosition
                                                                                        , xVelocity
                                                                                        , yVelocity
                                                                                        , zVelocity
                                                                                        );

            projectile[_final_index]->timer = 0;
            projectile[_final_index]->x = enemy[i]->x+enemy[i]->vx;
            projectile[_final_index]->y = enemy[i]->y+enemy[i]->vy;
            projectile[_final_index]->z = enemy[i]->z+enemy[i]->vz;
            //float dist = sqrt(pow(xPosition-enemy[i]->x,2)+pow(yPosition-enemy[i]->y,2)+pow(zPosition-enemy[i]->z,2));

            projectile[_final_index]->vx = (xPosition+intersection_time*xVelocity+.5*intersection_time*intersection_time*(xVelocity-xVelocity_prev)-enemy[i]->x);
            projectile[_final_index]->vy = (yPosition+intersection_time*yVelocity+.5*intersection_time*intersection_time*(yVelocity-yVelocity_prev)-enemy[i]->y);
            projectile[_final_index]->vz = (zPosition+intersection_time*zVelocity+.5*intersection_time*intersection_time*(zVelocity-zVelocity_prev)-enemy[i]->z);
            float vel = sqrt(projectile[_final_index]->vx*projectile[_final_index]->vx
                            +projectile[_final_index]->vy*projectile[_final_index]->vy
                            +projectile[_final_index]->vz*projectile[_final_index]->vz
                            );
            if(vel > 1e-5)
            {
                projectile[_final_index]->vx/=vel;
                projectile[_final_index]->vy/=vel;
                projectile[_final_index]->vz/=vel;
            }
            projectile[_final_index]->vx*=0.5;
            projectile[_final_index]->vy*=0.5;
            projectile[_final_index]->vz*=0.5;
            projectile[_final_index]->start_time = _curr_time;
            projectile[_final_index]->end_time = _curr_time+1;
            projectile[_final_index]->live = true;
            _final_index++;
            if(_final_index >= projectile.size())
                _final_index = 0;
        }
    }
    

    glTranslatef( -xPosition
                , -yPosition
                , -zPosition
                ); 

    draw  (projectile,_curr_time,_curr_index,_final_index);
    draw  (mining_projectile,_mining_curr_time,_mining_curr_index,_mining_final_index);
    draw  (resource_projectile,_resources_curr_time,_resources_curr_index,_resources_final_index);
    sun_generator->draw();

    glPushMatrix();
    glTranslatef(400,400,400);
    glScalef(50,50,50);
    glBindTexture( GL_TEXTURE_2D, texture[0] );
    DrawMesh(space_station_cmesh);
    glPopMatrix();

    update(projectile,_curr_time,_curr_index,_final_index);
    update(mining_projectile,_mining_curr_time,_mining_curr_index,_mining_final_index);
    update(resource_projectile,_resources_curr_time,_resources_curr_index,_resources_final_index);

    for(int i=0;i<enemy.size();i++)
    {
        if(enemy[i]->live)
        {
            enemy[i]->draw();
        }
    }

    int x,y,z;
    bool draw;

    glPushMatrix();

    glMultMatrixf(asteroid_m);

    ////std::cout << blocks.size() << std::endl;

    glCullFace(GL_BACK);


    if(update_scene){

    for(int i=0;i<blocks.size();i++)
    if(blocks[i]->type > 0){

        int x = blocks[i]->x;
        int y = blocks[i]->y;
        int z = blocks[i]->z;

        bool draw = false;

        if(x == 0 )
            draw = true;
        if(y == 0 )
            draw = true;
        if(z == 0 )
            draw = true;
        if(x == 99)
            draw = true;
        if(y == 99)
            draw = true;
        if(z == 99)
            draw = true;

        if(draw == false){
            draw = (data[x+1][y][z] == NULL)
                || (data[x-1][y][z] == NULL)
                || (data[x][y+1][z] == NULL)
                || (data[x][y-1][z] == NULL)
                || (data[x][y][z+1] == NULL)
                || (data[x][y][z-1] == NULL)

                || (data[x+1][y+1][z+1] == NULL)
                || (data[x-1][y+1][z+1] == NULL)
                || (data[x+1][y-1][z+1] == NULL)
                || (data[x-1][y-1][z+1] == NULL)
                || (data[x+1][y+1][z-1] == NULL)
                || (data[x-1][y+1][z-1] == NULL)
                || (data[x+1][y-1][z-1] == NULL)
                || (data[x-1][y-1][z-1] == NULL)

                || (data[x+1][y+1][z] == NULL)
                || (data[x-1][y+1][z] == NULL)
                || (data[x+1][y-1][z] == NULL)
                || (data[x-1][y-1][z] == NULL)

                || (data[x+1][y][z+1] == NULL)
                || (data[x-1][y][z+1] == NULL)
                || (data[x+1][y][z-1] == NULL)
                || (data[x-1][y][z-1] == NULL)

                || (data[x][y+1][z+1] == NULL)
                || (data[x][y-1][z+1] == NULL)
                || (data[x][y+1][z-1] == NULL)
                || (data[x][y-1][z-1] == NULL)

                 ;
        }

        if(blocks[i]->visible)
            draw = false;

        if(draw) {


            mesh.AddBox( blocks[i]->x-50
                       , blocks[i]->y-50
                       , blocks[i]->z-50
                       , 0.5
                       , 0.5
                       , 0.5
                       , blocks[i]->type
                       , blocks[i]
                       );


        }

    }
    }


    static int shadow_timer = 2000;
    shadow_timer++;

    if(update_scene || shadow_timer > 3) 
    {
        shadow_timer=0;

    for(int i=0;i<blocks.size();i++)
        if(blocks[i]->health < 100)
            blocks[i]->health += 2;

    for(int i=0;i<blocks.size();i++)
        if(blocks[i]->type == 11)
            blocks[i]->artificial_light = 1.0;
        else
            blocks[i]->artificial_light = 0.0;


    for(int i=0;i<blocks.size();i++){

        int x = blocks[i]->x;
        int y = blocks[i]->y;
        int z = blocks[i]->z;

        bool draw = false;

        if(x == 0 )
            draw = true;
        if(y == 0 )
            draw = true;
        if(z == 0 )
            draw = true;
        if(x == 99)
            draw = true;
        if(y == 99)
            draw = true;
        if(z == 99)
            draw = true;

        if(draw == false){
            draw = (data[x+1][y][z] == NULL || data[x+1][y][z]->type == 0)
                || (data[x-1][y][z] == NULL || data[x-1][y][z]->type == 0)
                || (data[x][y+1][z] == NULL || data[x][y+1][z]->type == 0)
                || (data[x][y-1][z] == NULL || data[x][y-1][z]->type == 0)
                || (data[x][y][z+1] == NULL || data[x][y][z+1]->type == 0)
                || (data[x][y][z-1] == NULL || data[x][y][z-1]->type == 0)

                || (data[x+1][y+1][z+1] == NULL || data[x+1][y+1][z+1]->type == 0) 
                || (data[x-1][y+1][z+1] == NULL || data[x-1][y+1][z+1]->type == 0)
                || (data[x+1][y-1][z+1] == NULL || data[x+1][y-1][z+1]->type == 0)
                || (data[x-1][y-1][z+1] == NULL || data[x-1][y-1][z+1]->type == 0)
                || (data[x+1][y+1][z-1] == NULL || data[x+1][y+1][z-1]->type == 0)
                || (data[x-1][y+1][z-1] == NULL || data[x-1][y+1][z-1]->type == 0)
                || (data[x+1][y-1][z-1] == NULL || data[x+1][y-1][z-1]->type == 0)
                || (data[x-1][y-1][z-1] == NULL || data[x-1][y-1][z-1]->type == 0)

                || (data[x+1][y+1][z] == NULL || data[x+1][y+1][z]->type == 0)
                || (data[x-1][y+1][z] == NULL || data[x-1][y+1][z]->type == 0)
                || (data[x+1][y-1][z] == NULL || data[x+1][y-1][z]->type == 0)
                || (data[x-1][y-1][z] == NULL || data[x-1][y-1][z]->type == 0)

                || (data[x+1][y][z+1] == NULL || data[x+1][y][z+1]->type == 0)
                || (data[x-1][y][z+1] == NULL || data[x-1][y][z+1]->type == 0)
                || (data[x+1][y][z-1] == NULL || data[x+1][y][z-1]->type == 0)
                || (data[x-1][y][z-1] == NULL || data[x-1][y][z-1]->type == 0)

                || (data[x][y+1][z+1] == NULL || data[x][y+1][z+1]->type == 0)
                || (data[x][y-1][z+1] == NULL || data[x][y-1][z+1]->type == 0)
                || (data[x][y+1][z-1] == NULL || data[x][y+1][z-1]->type == 0)
                || (data[x][y-1][z-1] == NULL || data[x][y-1][z-1]->type == 0)

                 ;
        }

        if(draw) {


            if(blocks[i]->type == 11)
            if(blocks[i]->power > .5){
                
                for(int _x=-4;_x<=4;_x+=1)
                for(int _y=-4;_y<=4;_y+=1)
                for(int _z=-4;_z<=4;_z+=1)
                if(data[x+_x][y+_y][z+_z] != NULL)
                if(data[x+_x][y+_y][z+_z]->visible == true)
                data[x+_x][y+_y][z+_z]->artificial_light += 0.15*(6.5-sqrt(_x*_x+_y*_y+_z*_z));

            }

            if(blocks[i]->artificial_light < 0.0)
                blocks[i]->artificial_light = 0.0;

            if(blocks[i]->artificial_light > 1.0)
                blocks[i]->artificial_light = 1.0;

        }

    }


    for(int i=0;i<blocks.size();i++) 
    {

        int x = blocks[i]->x;
        int y = blocks[i]->y;
        int z = blocks[i]->z;

        bool draw = false;

        if(x == 0 )
            draw = true;
        if(y == 0 )
            draw = true;
        if(z == 0 )
            draw = true;
        if(x == 99)
            draw = true;
        if(y == 99)
            draw = true;
        if(z == 99)
            draw = true;

        if(draw == false){
            draw = (data[x+1][y][z] == NULL || data[x+1][y][z]->type == 0)
                || (data[x-1][y][z] == NULL || data[x-1][y][z]->type == 0)
                || (data[x][y+1][z] == NULL || data[x][y+1][z]->type == 0)
                || (data[x][y-1][z] == NULL || data[x][y-1][z]->type == 0)
                || (data[x][y][z+1] == NULL || data[x][y][z+1]->type == 0)
                || (data[x][y][z-1] == NULL || data[x][y][z-1]->type == 0)

                || (data[x+1][y+1][z+1] == NULL || data[x+1][y+1][z+1]->type == 0) 
                || (data[x-1][y+1][z+1] == NULL || data[x-1][y+1][z+1]->type == 0)
                || (data[x+1][y-1][z+1] == NULL || data[x+1][y-1][z+1]->type == 0)
                || (data[x-1][y-1][z+1] == NULL || data[x-1][y-1][z+1]->type == 0)
                || (data[x+1][y+1][z-1] == NULL || data[x+1][y+1][z-1]->type == 0)
                || (data[x-1][y+1][z-1] == NULL || data[x-1][y+1][z-1]->type == 0)
                || (data[x+1][y-1][z-1] == NULL || data[x+1][y-1][z-1]->type == 0)
                || (data[x-1][y-1][z-1] == NULL || data[x-1][y-1][z-1]->type == 0)

                || (data[x+1][y+1][z] == NULL || data[x+1][y+1][z]->type == 0)
                || (data[x-1][y+1][z] == NULL || data[x-1][y+1][z]->type == 0)
                || (data[x+1][y-1][z] == NULL || data[x+1][y-1][z]->type == 0)
                || (data[x-1][y-1][z] == NULL || data[x-1][y-1][z]->type == 0)

                || (data[x+1][y][z+1] == NULL || data[x+1][y][z+1]->type == 0)
                || (data[x-1][y][z+1] == NULL || data[x-1][y][z+1]->type == 0)
                || (data[x+1][y][z-1] == NULL || data[x+1][y][z-1]->type == 0)
                || (data[x-1][y][z-1] == NULL || data[x-1][y][z-1]->type == 0)

                || (data[x][y+1][z+1] == NULL || data[x][y+1][z+1]->type == 0)
                || (data[x][y-1][z+1] == NULL || data[x][y-1][z+1]->type == 0)
                || (data[x][y+1][z-1] == NULL || data[x][y+1][z-1]->type == 0)
                || (data[x][y-1][z-1] == NULL || data[x][y-1][z-1]->type == 0)

                 ;
        }

        if(draw) {


            blocks[i]->light = 1.0;//-((x-50)*asteroid_m[0]+(y-50)*asteroid_m[4]+(z-50)*asteroid_m[8])/10.0;
            double X = x;
            double Y = y;
            double Z = z;
            for(int k=1;k<50;k++){
                X -= asteroid_m[0];
                Y -= asteroid_m[4];
                Z -= asteroid_m[8];
                if(X<0||X>=100||Y<0||Y>=100||Z<0||Z>=100)
                    break;
                if(data[(int)X][(int)Y][(int)Z] != NULL)
                if(data[(int)X][(int)Y][(int)Z]->type != 0)
                if(data[(int)X][(int)Y][(int)Z]->type != 28)
                    blocks[i]->light -= .06;
                if(blocks[i]->light <= 0)
                    break;
            }

            if(blocks[i]->light <= 0.10)
                blocks[i]->light = 0.10;

            if(blocks[i]->type == 8){
                if(blocks[i]->light > 0.5){
                    blocks[i]->power = 1.0;
                }
            }


            if(blocks[i]->type == 16)
            if(blocks[i]->power > 5.0)
            if(blocks[i]->signal > 0.5)
            {
                blocks[i]->signal = 0;
                //apply_force(Block* b,float direction_x,float direction_y,float direction_z,float& tx,float& ty,float& tz,float& fx,float& fy,float& fz) // direction defined in local coordinate system
                float tx;
                float ty;
                float tz;
                float fx;
                float fy;
                float fz;
                float dir_x = 0;
                float dir_y = 0;
                float dir_z = 0;

                if( data[x+1][y][z] == NULL )   dir_x += 1.0;
                else 
                if( data[x+1][y][z]->type == 0 ) dir_x += 1.0;
 
                if( data[x-1][y][z] == NULL )   dir_x -= 1.0;
                else 
                if( data[x-1][y][z]->type == 0 ) dir_x -= 1.0;
 
                if( data[x][y+1][z] == NULL )   dir_y += 1.0;
                else 
                if( data[x][y+1][z]->type == 0 ) dir_y += 1.0;
 
                if( data[x][y-1][z] == NULL )   dir_y -= 1.0;
                else 
                if( data[x][y-1][z]->type == 0 ) dir_y -= 1.0;
  
                if( data[x][y][z+1] == NULL )   dir_z += 1.0;
                else 
                if( data[x][y][z+1]->type == 0 ) dir_z += 1.0;
 
                if( data[x][y][z-1] == NULL )   dir_z -= 1.0;
                else 
                if( data[x][y][z-1]->type == 0 ) dir_z -= 1.0;
    
                //std::cout << "dir_x=" << dir_x << " ";  
                //std::cout << "dir_y=" << dir_y << " ";  
                //std::cout << "dir_z=" << dir_z << " " << std::endl;  
   
                apply_force( blocks[i] 
                           , dir_x
                           , dir_y
                           , dir_z
                           , tx
                           , ty
                           , tz
                           , fx
                           , fy
                           , fz
                           );
                asteroid_wx += .01*tx;
                asteroid_wy += .01*ty;
                asteroid_wz += .01*tz;
                float t = sqrt(asteroid_wx*asteroid_wx+asteroid_wy*asteroid_wy+asteroid_wz*asteroid_wz);
                if(t > 0.001){
                    asteroid_wx = 0.001 * asteroid_wx / t;
                    asteroid_wy = 0.001 * asteroid_wy / t;
                    asteroid_wz = 0.001 * asteroid_wz / t;
                }
 
                //std::cout << "rot_x=" << asteroid_wx << " ";  
                //std::cout << "rot_y=" << asteroid_wy << " ";  
                //std::cout << "rot_z=" << asteroid_wz << " " << std::endl;  
 
                if(selection)
                if(selection->type == 17)
                if(selection->menu)
                if(selection->menu->component.size() >= 22){
                    //std::cout << "selection_component_size=" << selection->menu->component.size() << std::endl; 
                    {
                        std::stringstream ss;
                        ss << asteroid_wx;
                        selection->menu->component[19]->name = ss.str(); 
                    }
                    {
                        std::stringstream ss;
                        ss << asteroid_wy;
                        selection->menu->component[20]->name = ss.str(); 
                    }
                    {
                        std::stringstream ss;
                        ss << asteroid_wz;
                        selection->menu->component[21]->name = ss.str(); 
                    }
                }

            }           
 

            player->radiation = 0.0;
            if(blocks[i]->type == 30)
            if(blocks[i]->menu->input[0]->item != NULL)
            if(blocks[i]->menu->input[0]->item->type == 5)
            {

                if(rand()%1000000 < 10){

                    if(blocks[i]->menu->input[0]->item->quantity > 0)
                        blocks[i]->menu->input[0]->item->quantity--;
                    else{
                        delete blocks[i]->menu->input[0]->item;
                        blocks[i]->menu->input[0]->item = NULL;
                    }
                
                }

                blocks[i]->power = 100.0;  // generate power
                blocks[i]->heat += 0.01; // heat the generator
                //std::cout << blocks[i]->heat << std::endl;
                if(blocks[i]->heat > 1.0){ // emit radiation
                    if(blocks[i]->heat > 2.0)
                        blocks[i]->heat = 2.0;
                    double t_x = (xPosition - ((blocks[i]->x-50)*asteroid_m[0]+(blocks[i]->y-50)*asteroid_m[4]+(blocks[i]->z-50)*asteroid_m[8]));
                    double t_y = (yPosition - ((blocks[i]->x-50)*asteroid_m[1]+(blocks[i]->y-50)*asteroid_m[5]+(blocks[i]->z-50)*asteroid_m[9]));
                    double t_z = (zPosition - ((blocks[i]->x-50)*asteroid_m[2]+(blocks[i]->y-50)*asteroid_m[6]+(blocks[i]->z-50)*asteroid_m[10]));
                    double t_r = sqrt(t_x*t_x+t_y*t_y+t_z*t_z);
                    double t_R = t_r;
                    if(t_r > 1e-5){
                        t_x /= t_r;
                        t_y /= t_r;
                        t_z /= t_r;

                        double radiation = 1.0;

                        for(int t=1;t<50&&t_r > 1.0;t++){

                            t_r -= 1;

                            int X = (int)((xPosition+t*t_x+.5)*asteroid_m[0] + (yPosition+t*t_y+.5)*asteroid_m[1] + (zPosition+t*t_z+.5)*asteroid_m[2] )+50;
                            int Y = (int)((xPosition+t*t_x+.5)*asteroid_m[4] + (yPosition+t*t_y+.5)*asteroid_m[5] + (zPosition+t*t_z+.5)*asteroid_m[6] )+50;
                            int Z = (int)((xPosition+t*t_x+.5)*asteroid_m[8] + (yPosition+t*t_y+.5)*asteroid_m[9] + (zPosition+t*t_z+.5)*asteroid_m[10])+50;

                            if( X >= 2
                             && X <  97
                             && Y >= 2
                             && Y <  97
                             && Z >= 2
                             && Z <  97
                              )
                            {
                                //if(data[X][Y][Z]->type == 2)
                                {
                                    radiation -= 0.02;
                                }
                                if(radiation <= 0.0){
                                    radiation = 0.0;
                                    break;
                                }
                            }

                        }
                        double damage = 10.0*radiation/(t_R*t_R+1);
                        player->life -= damage;
                        player->radiation += damage;

                    }
                }
            }

            blocks[i]->light += blocks[i]->artificial_light;

            for(int j=0;j<blocks[i]->faces.size();j++){
                mesh.face[blocks[i]->faces[j]]->color_r = blocks[i]->light; //blocks[i]->power;
                mesh.face[blocks[i]->faces[j]]->color_g = blocks[i]->light * blocks[i]->health / 100.0;
                mesh.face[blocks[i]->faces[j]]->color_b = blocks[i]->light * blocks[i]->health / 100.0;
                if(blocks[i] == selection)
                mesh.face[blocks[i]->faces[j]]->color_g = blocks[i]->light * .5;
            }

            update_scene = true;

        }

    }

/*  
    item_array.push_back( new Item( 2, 1, "rock"                  , 2 ) );
    item_array.push_back( new Item( 3, 1, "iron"                  , 3 ) );
    item_array.push_back( new Item( 4, 1, "ice"                   , 4 ) );
    item_array.push_back( new Item( 5, 1, "uranium"               , 5 ) );
    item_array.push_back( new Item( 6, 1, "carbon"                , 6 ) );
    item_array.push_back( new Item( 7, 1, "hydrocarbon"           , 7 ) );

    item_array.push_back( new Item( 8, 5, "solar panel"           , 8 ) );
    item_array.push_back( new Item( 9, 2, "wire"                  , 9 ) );
    item_array.push_back( new Item(10,10, "capacitor"             ,10 ) );
    item_array.push_back( new Item(11, 5, "lamp"                  ,11 ) );

    item_array.push_back( new Item(12,10, "3D printer"            ,12 ) );
    item_array.push_back( new Item(13,10, "atmosphere generator"  ,13 ) );
    item_array.push_back( new Item(14,10, "container"             ,14 ) );
    item_array.push_back( new Item(15,10, "refinery"              ,15 ) );

    item_array.push_back( new Item(16,10, "door"                  ,16 ) );
    item_array.push_back( new Item(17,10, "control panel"         ,17 ) );
    item_array.push_back( new Item(18,10, "camera"                ,18 ) );
    item_array.push_back( new Item(19,10, "heat dissipator"       ,19 ) );

    item_array.push_back( new Item(20, 2, "stone"                 ,20 ) );
    item_array.push_back( new Item(21, 2, "metal"                 ,21 ) );
    item_array.push_back( new Item(22, 2, "ceramic"               ,22 ) );
    item_array.push_back( new Item(23, 2, "carbon fiber"          ,23 ) );

    item_array.push_back( new Item(24, 1, "water"                 ,24 ) );
    item_array.push_back( new Item(25, 1, "food"                  ,25 ) );
    item_array.push_back( new Item(26,10, "condenser"             ,26 ) );
    item_array.push_back( new Item(27,10, "greenhouse"            ,27 ) );

    item_array.push_back( new Item(28, 1, "window"                ,28 ) );
    item_array.push_back( new Item(29,10, "door"                  ,29 ) );
*/



    for(int i=0;i<blocks.size();i++)
        if(blocks[i]->type == 31)
        if(blocks[i]->power > 0.5)
        { // cooler
            int x = blocks[i]->x;
            int y = blocks[i]->y;
            int z = blocks[i]->z;
            double avg_heat;
            for(int _x=-1;_x<=1;_x++)
            for(int _y=-1;_y<=1;_y++)
            for(int _z=-1;_z<=1;_z++)
            if(_x*_x+_y*_y+_z*_z>0)
            if(data[x+_x][y+_y][z+_z] != NULL)
            {
                if(data[x+_x][y+_y][z+_z]->heat > 0.01){
                    avg_heat = blocks[i]->heat*.001 + data[x+_x][y+_y][z+_z]->heat*.999;
                    blocks[i]->heat = avg_heat;
                    data[x+_x][y+_y][z+_z]->heat = avg_heat;
                }else
                {
                    avg_heat = blocks[i]->heat*.999;
                    blocks[i]->heat = avg_heat;
                }
            }
        }



    for(int i=0;i<blocks.size();i++)
    if(blocks[i]->type == 9){ // wire
        int x = blocks[i]->x;
        int y = blocks[i]->y;
        int z = blocks[i]->z;
        double new_power = 0;
        for(int _x=-1;_x<=1;_x++)
        for(int _y=-1;_y<=1;_y++)
        for(int _z=-1;_z<=1;_z++)
        if(_x*_x+_y*_y+_z*_z==1)
        if(data[x+_x][y+_y][z+_z] != NULL)
        if(data[x+_x][y+_y][z+_z]->type == 8 || data[x+_x][y+_y][z+_z]->type == 30){ // solar panel     or     nuclear generator
            new_power += data[x+_x][y+_y][z+_z]->power;
        }
        if(new_power > 1e-5){
            blocks[i]->power = new_power;
        }
        for(int _x=-1;_x<=1;_x++)
        for(int _y=-1;_y<=1;_y++)
        for(int _z=-1;_z<=1;_z++)
        if(_x*_x+_y*_y+_z*_z==1)
        if(data[x+_x][y+_y][z+_z] != NULL)
        if(data[x+_x][y+_y][z+_z]->type == 9){ // another wire
            double tmp_power = .5*(data[x+_x][y+_y][z+_z]->power + blocks[i]->power);
            data[x+_x][y+_y][z+_z]->power = tmp_power;
            blocks[i]->power = tmp_power;
        }else
        if(data[x+_x][y+_y][z+_z]->type == 10){ // capacitor
            if(data[x+_x][y+_y][z+_z]->power < blocks[i]->power)
                data[x+_x][y+_y][z+_z]->power += (blocks[i]->power - data[x+_x][y+_y][z+_z]->power)*.01;
            else
                blocks[i]->power = data[x+_x][y+_y][z+_z]->power;
        }
        else
        if( data[x+_x][y+_y][z+_z]->type == 11 // utilities
         || data[x+_x][y+_y][z+_z]->type == 12
         || data[x+_x][y+_y][z+_z]->type == 13
         || data[x+_x][y+_y][z+_z]->type == 15
         || data[x+_x][y+_y][z+_z]->type == 26
         || data[x+_x][y+_y][z+_z]->type == 27
         || data[x+_x][y+_y][z+_z]->type == 31
          )
        { // appliances
            if(data[x+_x][y+_y][z+_z]->power < blocks[i]->power)
                data[x+_x][y+_y][z+_z]->power = blocks[i]->power;
        }
    }

/* 
    for(int i=0;i<blocks.size();i++){
        int x = blocks[i]->x;
        int y = blocks[i]->y;
        int z = blocks[i]->z;
        if(blocks[i]->type == 9 || blocks[i]->type == 11 || blocks[i]->type == 12 || blocks[i]->type == 13 || blocks[i]->type == 15 || blocks[i]->type == 26 || blocks[i]->type == 27)
            for(int _x=-1;_x<=1;_x++)
            for(int _y=-1;_y<=1;_y++)
            for(int _z=-1;_z<=1;_z++)
            if(_x*_x+_y*_y+_z*_z==1)
            if(data[x+_x][y+_y][z+_z] != NULL){
                    if(data[x+_x][y+_y][z+_z]->type == 8)
                        blocks[i]->power += data[x+_x][y+_y][z+_z]->power;
                    if(data[x+_x][y+_y][z+_z]->type == 10)
                    if(_x == 1 && _y == 0 && _z == 0)
                        blocks[i]->power += data[x+_x][y+_y][z+_z]->power;
 
            }
    }



    for(int i=0;i<blocks.size();i++){
        int x = blocks[i]->x;
        int y = blocks[i]->y;
        int z = blocks[i]->z;
        if(blocks[i]->type == 10 || blocks[i]->type == 9 || blocks[i]->type == 11 || blocks[i]->type == 12 || blocks[i]->type == 13 || blocks[i]->type == 15 || blocks[i]->type == 26 || blocks[i]->type == 27)
            for(int _x=-1;_x<=1;_x++)
            for(int _y=-1;_y<=1;_y++)
            for(int _z=-1;_z<=1;_z++)
            if(_x*_x+_y*_y+_z*_z==1)
            if(data[x+_x][y+_y][z+_z] != NULL)
            {
                if(blocks[i]->type != 10){
                    if(blocks[i]->power < data[x+_x][y+_y][z+_z]->power)
                        if(data[x+_x][y+_y][z+_z]->type == 9)
                            blocks[i]->power = .9*data[x+_x][y+_y][z+_z]->power;
                }else{
                    if(_x == 1 && _y == 0 && _z == 0)
                    if(data[x+_x][y+_y][z+_z]->power > 0.0){
                        //std::cout << "pass 1" << std::endl;
                        if(blocks[i]->power < 0.5/data[x+_x][y+_y][z+_z]->power && data[x+_x][y+_y][z+_z]->power > 0.25){
                            //std::cout << "pass 2" << std::endl;
                            if(data[x+_x][y+_y][z+_z]->type == 9){
                                //std::cout << "pass 3" << std::endl;
                                //std::cout << "power=" << blocks[i]->power << std::endl;
                                blocks[i]->power = 0.5/data[x+_x][y+_y][z+_z]->power;
                                if(blocks[i]->power > 0.75)
                                    blocks[i]->power = 0.75;
                            }
                        }
                    }
                }
            }
    }
*/

    for(int i=0;i<blocks.size();i++){
        if(blocks[i]->type == 10)
            blocks[i]->power *= 0.99995;
        else
            blocks[i]->power *= 0.995;
    }

    for(int i=0;i<blocks.size();i++)
    if(blocks[i]->type == 26)
    if(blocks[i]->power > 0.5){
    ////std::cout << blocks[i]->air << std::endl;
    if(blocks[i]->air > 5.0){
    if(rand()%10000 < 10){
    
        if(blocks[i]->menu->output[0]->item == NULL){
            blocks[i]->menu->output[0]->item = new Item(*item_array[24]);
            blocks[i]->menu->output[0]->item->quantity = 1;
        }else
        {
            blocks[i]->menu->output[0]->item->quantity++;
        }
    
    }
    }
    }

    for(int i=0;i<blocks.size();i++)
    if(blocks[i]->type == 27)
    if(blocks[i]->power > 0.5)
    if(blocks[i]->air > 5.0)
    if(blocks[i]->light > 0.5)
    if(blocks[i]->menu->input[0]->item != NULL)
    if(blocks[i]->menu->input[0]->item->type == 24){
    if(rand()%20000 < 10){
    
        if(blocks[i]->menu->output[0]->item == NULL){
            blocks[i]->menu->output[0]->item = new Item(*item_array[25]);
            blocks[i]->menu->output[0]->item->quantity = 1;
        }else
        {
            blocks[i]->menu->output[0]->item->quantity++;
        }

        if(blocks[i]->menu->input[0]->item->quantity > 0)
            blocks[i]->menu->input[0]->item->quantity--;
        else{
            delete blocks[i]->menu->input[0]->item;
            blocks[i]->menu->input[0]->item = NULL;
        }
    
    }
    }


    }
    

    for(int i=0;i<blocks.size();i++)
    if(blocks[i]->type == 13 || blocks[i]->type == 0){
        if(blocks[i]->air > 10.0)
            blocks[i]->air = 10.0;
        if(blocks[i]->type == 13 && blocks[i]->power > 0.5)
            blocks[i]->air = 10.0;
        int x = blocks[i]->x;
        int y = blocks[i]->y;
        int z = blocks[i]->z;
            if(blocks[i]->air > 1.0)
            for(int _x=-1;_x<=1;_x++)
            for(int _y=-1;_y<=1;_y++)
            for(int _z=-1;_z<=1;_z++)
            if((_x*_x+_y*_y+_z*_z)==1)
            if(x+_x >= 0 && x+_x < 100 && 
               y+_y >= 0 && y+_y < 100 && 
               z+_z >= 0 && z+_z < 100){
            if( data[x+_x][y+_y][z+_z] == NULL ){
                Block* b = add_block(x+_x,y+_y,z+_z,0);
                ////std::cout << "add block " << x+_x << " " << y+_y << " " << z+_z << std::endl; 
                b->air = .1*blocks[i]->air;
                blocks[i]->air -= .1*blocks[i]->air;
/* 
            mesh.AddBox( x+_x-50
                       , y+_y-50
                       , z+_z-50
                       , 0.5
                       , 0.5
                       , 0.5
                       , 0
                       , b
                       );
*/

            }
            }
    }

    ////std::cout << blocks.size() << std::endl;

    for(int i=0;i<blocks.size();i++)
    if(blocks[i]->type == 13 || blocks[i]->type == 0 || blocks[i]->type == 26 || blocks[i]->type == 27){
        blocks[i]->air *= .995;
        if(blocks[i]->air > 10.0)
            blocks[i]->air = 10.0;
        int x = blocks[i]->x;
        int y = blocks[i]->y;
        int z = blocks[i]->z;
        double avg;
            if(blocks[i]->air > 1.0)
            for(int _x=-1;_x<=1;_x++)
            for(int _y=-1;_y<=1;_y++)
            for(int _z=-1;_z<=1;_z++)
            if((_x*_x+_y*_y+_z*_z)==1)
            if(x+_x >= 0 && x+_x < 100 && 
               y+_y >= 0 && y+_y < 100 && 
               z+_z >= 0 && z+_z < 100)
            if(data[x+_x][y+_y][z+_z] != NULL)
            {
                if(data[x+_x][y+_y][z+_z]->type == 0 || data[x+_x][y+_y][z+_z]->type == 26 || data[x+_x][y+_y][z+_z]->type == 27)
                {
                avg = .5*(data[x+_x][y+_y][z+_z]->air+blocks[i]->air);
                data[x+_x][y+_y][z+_z]->air = avg;
                blocks[i]->air = avg;
                }
            }
    }
/*
    vector<Block*> t_blocks = blocks;
    for(int i=0;i<t_blocks.size();i++)
        if(t_blocks[i]->type == 0)
        if(t_blocks[i]->air < 0.01){
            delete_block(t_blocks[i]->x,t_blocks[i]->y,t_blocks[i]->z);
        }
*/    

    if(update_scene){ 
        cmesh->LoadMesh(&mesh);
        cmesh->UpdateVBOs();
        update_scene = false;
    }

    glBindTexture( GL_TEXTURE_2D, texture[0] );

    DrawMesh(cmesh);

    glPopMatrix();

    glPopMatrix();

    glDisable(GL_DEPTH_TEST);
    {
        stringstream ss;
        ss << ceil(player->life) << "%";
        draw_text(0.5+0.025,0.3-0.005,-1,ss.str());
    }
    {
        stringstream ss;
        ss << player->armor;
        draw_text(0.5+0.025,0.3-0.005-0.025,-1,ss.str());
    }
    {
        stringstream ss;
        ss << (player->oxygen) << "% ";
        ss << "(" << (int)(player->air) << ")";
        draw_text(0.5+0.025,0.3-0.005-2*0.025,-1,ss.str());
    }
    {
        stringstream ss;
        ss << ceil(player->water) << "%";
        draw_text(0.5+0.025,0.3-0.005-3*0.025,-1,ss.str());
    }
    {
        stringstream ss;
        ss << ceil(player->food) << "%";
        draw_text(0.5+0.025,0.3-0.005-4*0.025,-1,ss.str());
    }
    {
        stringstream ss;
        ss << player->radiation;
        draw_text(0.5+0.025,0.3-0.005-5*0.025,-1,ss.str());
    }

    glEnable(GL_DEPTH_TEST);
 
    if(player->tool.v.size()>0){
    glDisable(GL_DEPTH_TEST);

    {
        stringstream ss;
        ss << "storage: " << 100.0f*(float)player->tool.curr_volume/player->tool.max_volume << "%";
        draw_text(-0.5,0.3+0.025,-1,ss.str());
    }
    for(int i=0;i<player->tool.v.size();i++){
        if(player->place_selection<0)
            player->place_selection+=player->tool.v.size();
        stringstream ss;
        ss << player->tool.v[(i+player->place_selection)%player->tool.v.size()]->quantity << "   " << player->tool.v[(i+player->place_selection)%player->tool.v.size()]->name;
        draw_text(-0.48,0.3-0.005-.025*i,-1,ss.str());
    }
    glBindTexture( GL_TEXTURE_2D, texture[0] );
    glBegin(GL_QUADS);
    ////std::cout << "size=" << player->tool.v.size() << std::endl;
    for(int i=0;i<player->tool.v.size();i++){
    if(player->place_selection<0)
        player->place_selection+=player->tool.v.size();
    int texture = (player->tool.v[(i+player->place_selection)%player->tool.v.size()]->texture);
    float su = .125*(texture%8), eu=su+.125, sv=.125*(texture/8), ev=sv+.125;
    glTexCoord2d(su,sv);
    glVertex3f(-0.5+.01,0.3+.01-.025*i,-1);
    glTexCoord2d(eu,sv);
    glVertex3f(-0.5-.01,0.3+.01-.025*i,-1);
    glTexCoord2d(eu,ev);
    glVertex3f(-0.5-.01,0.3-.01-.025*i,-1);
    glTexCoord2d(su,ev);
    glVertex3f(-0.5+.01,0.3-.01-.025*i,-1);
    }
    glEnd();




    glEnable(GL_DEPTH_TEST);
    }


    glDisable(GL_DEPTH_TEST);

    {
    glBegin(GL_QUADS);
    for(int i=0;i<6;i++){
    float su=i*0.015625, eu=su+0.015625, sv=1.0-0.015625, ev=sv+0.015625;
    glTexCoord2d(eu,sv);
    glVertex3f(0.5+.01,0.3+.01-.025*i,-1);
    glTexCoord2d(su,sv);
    glVertex3f(0.5-.01,0.3+.01-.025*i,-1);
    glTexCoord2d(su,ev);
    glVertex3f(0.5-.01,0.3-.01-.025*i,-1);
    glTexCoord2d(eu,ev);
    glVertex3f(0.5+.01,0.3-.01-.025*i,-1);
    }
    glEnd();
    }

    if(MENU)
    if(selection)
    if(selection->menu)
    selection->menu->draw();


    glBegin(GL_LINES);

    {

        glTexCoord2d(0.95,0.95);

        double dx = (double)(mouse_x - _WIDTH/2)/(WIDTH*8.0*.3);
        double dy = -(double)(mouse_y - _HEIGHT/2)/(WIDTH*8.0*.3);

        glVertex3f(dx+.01,dy,-1);
        glVertex3f(dx-.01,dy,-1);
        glVertex3f(dx,dy+.01,-1);
        glVertex3f(dx,dy-.01,-1);

    }

    glEnd();

    glEnable(GL_DEPTH_TEST);


    usleep(100);

    glutSwapBuffers();

    static int record_timer = 0;
    if(RECORD)
    {
        record_timer++;
        if(record_timer > 4)
        {
            record_timer = 0;
            screen_recorder.TakeScreenshot();
        }
    }

}


void animation(void)
{


    usleep(0);

    if(selection)
    if(!selection->menu)
    MENU = false;
    
    if(MENU){
    if(selection)
    if(selection->menu)
    if( (selection->type == 12 && selection->power > 0.5) 
     || (selection->type == 15 && selection->power > 0.5)
     || (selection->type == 13 && selection->power > 0.5)
     || (selection->type == 26 && selection->power > 0.5)
     || (selection->type == 27 && selection->power > 0.5)
     || (selection->type == 14)
     || (selection->type == 16 && selection->power > 5.0)
     || (selection->type == 17 && selection->power > 5.0)
     || (selection->type == 30)
    ){
    if(MINE){

        double dx = (double)(mouse_x - _WIDTH/2)/(WIDTH*7.5*.3);
        double dy = -(double)(mouse_y - _HEIGHT/2)/(WIDTH*7.5*.3);

        //std::cout << dx << " " << dy << std::endl;

        MenuComponent* mc = selection->menu->pick(dx,dy);

        if(mc){
            MINE = false;
            mc->picked = true;
            mc->update(player,1,(SHIFT)?64:1);
        }

    }else{

        selection->menu->unselect();

    }

    if(PLACE){

        double dx = (double)(mouse_x - _WIDTH/2)/(WIDTH*7.5*.3);
        double dy = -(double)(mouse_y - _HEIGHT/2)/(WIDTH*7.5*.3);

        //std::cout << dx << " " << dy << std::endl;

        MenuComponent* mc = selection->menu->pick(dx,dy);

        if(mc){
            PLACE = false;
            mc->picked = true;
            mc->update(player,2,(SHIFT)?64:1);
        }

    }else{

        selection->menu->unselect();

    }
    }else{
        MENU = false;
    }
    MINE = false;
    PLACE = false;
    }

/*
    for(int i=0;i<blocks.size();i++) 
    {

        glBindTexture( GL_TEXTURE_2D, texture[blocks[i]->type] );

        x = blocks[i]->x;
        y = blocks[i]->y;
        z = blocks[i]->z;

        draw = false;

        if(x == 0 )
            draw = true;
        if(y == 0 )
            draw = true;
        if(z == 0 )
            draw = true;
        if(x == 99)
            draw = true;
        if(y == 99)
            draw = true;
        if(z == 99)
            draw = true;

        if(draw == false){
            draw = (data[x+1][y][z] == NULL)
                || (data[x-1][y][z] == NULL)
                || (data[x][y+1][z] == NULL)
                || (data[x][y-1][z] == NULL)
                || (data[x][y][z+1] == NULL)
                || (data[x][y][z-1] == NULL)
                 ;
        }

        if(draw) {

            glPushMatrix();
       
            glTranslatef( x - 50
                        , y - 50
                        , z - 50
                        );
        
         
            glBegin(GL_QUADS);        // Draw The Cube Using quads
            //if((x-50)*asteroid_m[0]+(y-50)*asteroid_m[1]+(z-50)*asteroid_m[2] > 0)
            //    blocks[i]->light = 1.0;
            //else
            //    blocks[i]->light = 0.2;
            //if(rand()%50 == 0)
            //blocks[i]->light = 0.1;
            //if( data[x+1][y][z] == NULL 
            // && data[x+2][y][z] == NULL
            // && data[x+3][y][z] == NULL
            // && data[x+4][y][z] == NULL
            // && data[x+5][y][z] == NULL
            //  )
            //    blocks[i]->light = 1.0;
            GLfloat light = blocks[i]->light;
            glColor3f(light,light,light);    // Color
            glTexCoord2d(0.0,0.0); glVertex3f( scale, scale,-scale);    // Top Right Of The Quad (Top)
            glTexCoord2d(1.0,0.0); glVertex3f(-scale, scale,-scale);    // Top Left Of The Quad (Top)
            glTexCoord2d(1.0,1.0); glVertex3f(-scale, scale, scale);    // Bottom Left Of The Quad (Top)
            glTexCoord2d(0.0,1.0); glVertex3f( scale, scale, scale);    // Bottom Right Of The Quad (Top)
            glTexCoord2d(0.0,0.0); glVertex3f( scale,-scale, scale);    // Top Right Of The Quad (Bottom)
            glTexCoord2d(1.0,0.0); glVertex3f(-scale,-scale, scale);    // Top Left Of The Quad (Bottom)
            glTexCoord2d(1.0,1.0); glVertex3f(-scale,-scale,-scale);    // Bottom Left Of The Quad (Bottom)
            glTexCoord2d(0.0,1.0); glVertex3f( scale,-scale,-scale);    // Bottom Right Of The Quad (Bottom)
            glTexCoord2d(0.0,0.0); glVertex3f( scale, scale, scale);    // Top Right Of The Quad (Front)
            glTexCoord2d(1.0,0.0); glVertex3f(-scale, scale, scale);    // Top Left Of The Quad (Front)
            glTexCoord2d(1.0,1.0); glVertex3f(-scale,-scale, scale);    // Bottom Left Of The Quad (Front)
            glTexCoord2d(0.0,1.0); glVertex3f( scale,-scale, scale);    // Bottom Right Of The Quad (Front)
            glTexCoord2d(0.0,0.0); glVertex3f( scale,-scale,-scale);    // Top Right Of The Quad (Back)
            glTexCoord2d(1.0,0.0); glVertex3f(-scale,-scale,-scale);    // Top Left Of The Quad (Back)
            glTexCoord2d(1.0,1.0); glVertex3f(-scale, scale,-scale);    // Bottom Left Of The Quad (Back)
            glTexCoord2d(0.0,1.0); glVertex3f( scale, scale,-scale);    // Bottom Right Of The Quad (Back)
            glTexCoord2d(0.0,0.0); glVertex3f(-scale, scale, scale);    // Top Right Of The Quad (Left)
            glTexCoord2d(1.0,0.0); glVertex3f(-scale, scale,-scale);    // Top Left Of The Quad (Left)
            glTexCoord2d(1.0,1.0); glVertex3f(-scale,-scale,-scale);    // Bottom Left Of The Quad (Left)
            glTexCoord2d(0.0,1.0); glVertex3f(-scale,-scale, scale);    // Bottom Right Of The Quad (Left)
            glTexCoord2d(0.0,0.0); glVertex3f( scale, scale,-scale);    // Top Right Of The Quad (Right)
            glTexCoord2d(1.0,0.0); glVertex3f( scale, scale, scale);    // Top Left Of The Quad (Right)
            glTexCoord2d(1.0,1.0); glVertex3f( scale,-scale, scale);    // Bottom Left Of The Quad (Right)
            glTexCoord2d(0.0,1.0); glVertex3f( scale,-scale,-scale);    // Bottom Right Of The Quad (Right)
            glEnd();            // End Drawing The Cube
    
            glPopMatrix();    

        }

    }
*/

/*  
    {
    for(int i=0;i<blocks.size();i++) 
    {

        int x = blocks[i]->x;
        int y = blocks[i]->y;
        int z = blocks[i]->z;

        bool draw = false;

        if(x == 0 )
            draw = true;
        if(y == 0 )
            draw = true;
        if(z == 0 )
            draw = true;
        if(x == 99)
            draw = true;
        if(y == 99)
            draw = true;
        if(z == 99)
            draw = true;

        if(draw == false){
            draw = (data[x+1][y][z] == NULL)
                || (data[x-1][y][z] == NULL)
                || (data[x][y+1][z] == NULL)
                || (data[x][y-1][z] == NULL)
                || (data[x][y][z+1] == NULL)
                || (data[x][y][z-1] == NULL)
                 ;
        }

        if(draw) {


            blocks[i]->light = 1.0;//-((x-50)*asteroid_m[0]+(y-50)*asteroid_m[4]+(z-50)*asteroid_m[8])/10.0;
            double X = x;
            double Y = y;
            double Z = z;
            for(int k=1;k<50;k++){
                X -= asteroid_m[0];
                Y -= asteroid_m[4];
                Z -= asteroid_m[8];
                if(X<0||X>=100||Y<0||Y>=100||Z<0||Z>=100)
                    break;
                if(data[(int)X][(int)Y][(int)Z] != NULL)
                    blocks[i]->light -= .05;
                if(blocks[i]->light <= 0)
                    break;
            }


        }

    }
    }
*/

    ////std::cout << xPosition << " " << yPosition << " " << zPosition << std::endl;

    bool forced = false;

    xVelocity_prev = xVelocity;
    yVelocity_prev = yVelocity;
    zVelocity_prev = zVelocity;

    if(RIGHT){
        xVelocity += .0005*cameraRotation[0];
        yVelocity += .0005*cameraRotation[4];
        zVelocity += .0005*cameraRotation[8];
        forced = true;
    }
    if(LEFT){
        xVelocity -= .0005*cameraRotation[0];
        yVelocity -= .0005*cameraRotation[4];
        zVelocity -= .0005*cameraRotation[8];
        forced = true;
    }
    if(UP){
        xVelocity += .0005*cameraRotation[1];
        yVelocity += .0005*cameraRotation[5];
        zVelocity += .0005*cameraRotation[9];
        forced = true;
    }
    if(DOWN){
        xVelocity -= .0005*cameraRotation[1];
        yVelocity -= .0005*cameraRotation[5];
        zVelocity -= .0005*cameraRotation[9];
        forced = true;
    }
    if(FORWARD){
        xVelocity -= .001*cameraRotation[2];
        yVelocity -= .001*cameraRotation[6];
        zVelocity -= .001*cameraRotation[10];
        forced = true;
    }
    if(BACKWARD){
        xVelocity += .001*cameraRotation[2];
        yVelocity += .001*cameraRotation[6];
        zVelocity += .001*cameraRotation[10];
        forced = true;
    }

    if(CONTROL_RIGHT){
        if(selection){
            if(selection->menu){
                std::map<std::string,std::vector<Block*> >::iterator it = selection->menu->vec.find("3");
                if(it != selection->menu->vec.end()){
                    for(int i=0;i<it->second.size();i++){
                        it->second[i]->signal = 1;
                    }
                }
            }
        }
    }
    if(CONTROL_LEFT){
        if(selection){
            if(selection->menu){
                std::map<std::string,std::vector<Block*> >::iterator it = selection->menu->vec.find("4");
                if(it != selection->menu->vec.end()){
                    for(int i=0;i<it->second.size();i++){
                        it->second[i]->signal = 1;
                    }
                }
            }
        }
    }
    if(CONTROL_UP){
        if(selection){
            if(selection->menu){
                std::map<std::string,std::vector<Block*> >::iterator it = selection->menu->vec.find("5");
                if(it != selection->menu->vec.end()){
                    for(int i=0;i<it->second.size();i++){
                        it->second[i]->signal = 1;
                    }
                }
            }
        }
    }
    if(CONTROL_DOWN){
        if(selection){
            if(selection->menu){
                std::map<std::string,std::vector<Block*> >::iterator it = selection->menu->vec.find("6");
                if(it != selection->menu->vec.end()){
                    for(int i=0;i<it->second.size();i++){
                        it->second[i]->signal = 1;
                    }
                }
            }
        }
    }
    if(CONTROL_FORWARD){
        if(selection){
            if(selection->menu){
                std::map<std::string,std::vector<Block*> >::iterator it = selection->menu->vec.find("1");
                if(it != selection->menu->vec.end()){
                    for(int i=0;i<it->second.size();i++){
                        it->second[i]->signal = 1;
                    }
                }
            }
        }
    }
    if(CONTROL_BACKWARD){
        if(selection){
            if(selection->menu){
                std::map<std::string,std::vector<Block*> >::iterator it = selection->menu->vec.find("2");
                if(it != selection->menu->vec.end()){
                    for(int i=0;i<it->second.size();i++){
                        it->second[i]->signal = 1;
                    }
                }
            }
        }
    }



    {
        double dist_x = 0 - xPosition;
        double dist_y = 0 - yPosition;
        double dist_z = 0 - zPosition;
        double dist = sqrt(dist_x*dist_x+dist_y*dist_y+dist_z*dist_z);
        if(dist > 40){
            xVelocity += .5*dist_x/(dist*dist*dist);
            yVelocity += .5*dist_y/(dist*dist*dist);
            zVelocity += .5*dist_z/(dist*dist*dist);
        } else {
            xVelocity += .5*dist_x/(40*40*40);
            yVelocity += .5*dist_y/(40*40*40);
            zVelocity += .5*dist_z/(40*40*40);
        }

    }

            if(MINE)
            {


                int type = player->tool.v[(player->place_selection)%player->tool.v.size()]->type;
                static int rotate = 0;
                switch( tool_selection )
                {
                    case MINING_TOOL_TYPE:
                    {
                        // create mining projectile(s)
                        mining_projectile[_mining_final_index]->timer = 0;
                        mining_projectile[_mining_final_index]->x = xPosition - .5*cameraRotation[2];
                        mining_projectile[_mining_final_index]->y = yPosition - .5*cameraRotation[6];
                        mining_projectile[_mining_final_index]->z = zPosition - .5*cameraRotation[10];

                        mining_projectile[_mining_final_index]->vx = xVelocity-.4*cameraRotation[2];
                        mining_projectile[_mining_final_index]->vy = yVelocity-.4*cameraRotation[6];
                        mining_projectile[_mining_final_index]->vz = zVelocity-.4*cameraRotation[10];

                        mining_projectile[_mining_final_index]->start_time = _mining_curr_time;
                        mining_projectile[_mining_final_index]->end_time   = _mining_curr_time+1;
                        mining_projectile[_mining_final_index]->live       = true;
                        _mining_final_index++;
                        if(_mining_final_index >= mining_projectile.size())
                            _mining_final_index = 0;

                        break;
                    }

                    
                    case WEAPON_TYPE:
                    {
                        // create weapon projectile(s)
                        projectile[_final_index]->timer = 0;
                        rotate = (rotate+1)%4;
                        switch(rotate)
                        {
                            case 0:
                                projectile[_final_index]->x = xPosition  - .5*cameraRotation[2] -.1*cameraRotation[1];
                                projectile[_final_index]->y = yPosition  - .5*cameraRotation[6] -.1*cameraRotation[5];
                                projectile[_final_index]->z = zPosition  - .5*cameraRotation[10]-.1*cameraRotation[9];
                                break;
                            case 1:
                                projectile[_final_index]->x = xPosition  - .5*cameraRotation[2] -.1*cameraRotation[0];
                                projectile[_final_index]->y = yPosition  - .5*cameraRotation[6] -.1*cameraRotation[4];
                                projectile[_final_index]->z = zPosition  - .5*cameraRotation[10]-.1*cameraRotation[8];
                                break;
                            case 2:
                                projectile[_final_index]->x = xPosition  - .5*cameraRotation[2] +.1*cameraRotation[1];
                                projectile[_final_index]->y = yPosition  - .5*cameraRotation[6] +.1*cameraRotation[5];
                                projectile[_final_index]->z = zPosition  - .5*cameraRotation[10]+.1*cameraRotation[9];
                                break;
                            case 3:
                                projectile[_final_index]->x = xPosition  - .5*cameraRotation[2] +.1*cameraRotation[0];
                                projectile[_final_index]->y = yPosition  - .5*cameraRotation[6] +.1*cameraRotation[4];
                                projectile[_final_index]->z = zPosition  - .5*cameraRotation[10]+.1*cameraRotation[8];
                                break;
                        }
 
                        int min_ind = -1;
                        float min_dist = 30,tmp_dist;

                        for(int i=0;i<enemy.size();i++)
                        {
                            if(enemy[i]->live)
                            {
                                tmp_dist = sqrt(pow(enemy[i]->x-xPosition,2)+pow(enemy[i]->y-yPosition,2)+pow(enemy[i]->z-zPosition,2));
                                if(cameraRotation[2]*(enemy[i]->x-xPosition)+cameraRotation[6]*(enemy[i]->y-yPosition)+cameraRotation[10]*(enemy[i]->z-zPosition) < -tmp_dist * .75)
                                if(tmp_dist < min_dist)
                                {
                                    min_dist = tmp_dist;
                                    min_ind = i;
                                }
                            }
                        }

                        if(min_ind >= 0)
                        {
                            float intersection_time = calculate_intersection_time_between_ray_and_sphere( xPosition+xVelocity
                                                                                                        , yPosition+yVelocity
                                                                                                        , zPosition+zVelocity
                                                                                                        , 0.5
                                                                                                        , enemy[min_ind]->x
                                                                                                        , enemy[min_ind]->y
                                                                                                        , enemy[min_ind]->z
                                                                                                        , enemy[min_ind]->vx
                                                                                                        , enemy[min_ind]->vy
                                                                                                        , enemy[min_ind]->vz
                                                                                                        );


                            projectile[_final_index]->vx = (-xPosition+intersection_time*enemy[min_ind]->vx+enemy[min_ind]->x);
                            projectile[_final_index]->vy = (-yPosition+intersection_time*enemy[min_ind]->vy+enemy[min_ind]->y);
                            projectile[_final_index]->vz = (-zPosition+intersection_time*enemy[min_ind]->vz+enemy[min_ind]->z);
                            float vel = sqrt(projectile[_final_index]->vx*projectile[_final_index]->vx
                                            +projectile[_final_index]->vy*projectile[_final_index]->vy
                                            +projectile[_final_index]->vz*projectile[_final_index]->vz
                                            );
                            if(vel > 1e-5)
                            {
                                projectile[_final_index]->vx/=vel;
                                projectile[_final_index]->vy/=vel;
                                projectile[_final_index]->vz/=vel;
                            }
                            projectile[_final_index]->vx*=0.5;
                            projectile[_final_index]->vy*=0.5;
                            projectile[_final_index]->vz*=0.5;

                        }
                        else
                        {

                            float dx = float(mouse_x-_WIDTH/2)/(WIDTH*2);
                            float dy = float(mouse_y-_HEIGHT/2)/(WIDTH*2);

                            projectile[_final_index]->vx = xVelocity - .5*(cameraRotation[2] +dy*cameraRotation[1]-dx*cameraRotation[0]);
                            projectile[_final_index]->vy = yVelocity - .5*(cameraRotation[6] +dy*cameraRotation[5]-dx*cameraRotation[4]);
                            projectile[_final_index]->vz = zVelocity - .5*(cameraRotation[10]+dy*cameraRotation[9]-dx*cameraRotation[8]);

                        }

                        projectile[_final_index]->start_time = _curr_time;
                        projectile[_final_index]->end_time   = _curr_time+1;
                        projectile[_final_index]->live       = true;
                        _final_index++;
                        if(_final_index >= projectile.size())
                            _final_index = 0;

                        break;
                    }
                   
                    default:
                    {
                        // do nothing
                    }
                }


//                int __x = points[i]->x;
//                int __y = points[i]->y;
//                int __z = points[i]->z;

//                int damage = 20;
//                if( data[points[i]->x][points[i]->y][points[i]->z]->armor <= damage)
//                data[points[i]->x][points[i]->y][points[i]->z]->health -= damage - data[points[i]->x][points[i]->y][points[i]->z]->armor;
//                if(data[points[i]->x][points[i]->y][points[i]->z]->health < 0){

//                if(player->tool.insert_item(data[points[i]->x][points[i]->y][points[i]->z]->type) == 0){

//                mesh.RemoveBox( data[points[i]->x][points[i]->y][points[i]->z] );
//                delete_block( points[i]->x
//                            , points[i]->y
//                            , points[i]->z
//                            );
//                }
//                }
/* 
                for(int _x=-1;_x<=1;_x+=2)
                for(int _y=-1;_y<=1;_y+=2)
                for(int _z=-1;_z<=1;_z+=2){
                Block* block = data[__x+_x][__y+_y][__z+_z];
                if(block != NULL)
                if(block->visible == false)
                mesh.AddBox( block->x-50
                           , block->y-50
                           , block->z-50
                           , 0.5
                           , 0.5
                           , 0.5
                           , block->type
                           , block
                           );
                }
*/


//                cmesh->LoadMesh(&mesh);
//                cmesh->UpdateVBOs();

//                update_scene = true;

            }


    int X,Y,Z;

    X = (int)((xPosition+.5)*asteroid_m[0] + (yPosition+.5)*asteroid_m[1] + (zPosition+.5)*asteroid_m[2] )+50;
    Y = (int)((xPosition+.5)*asteroid_m[4] + (yPosition+.5)*asteroid_m[5] + (zPosition+.5)*asteroid_m[6] )+50;
    Z = (int)((xPosition+.5)*asteroid_m[8] + (yPosition+.5)*asteroid_m[9] + (zPosition+.5)*asteroid_m[10])+50;

    ////std::cout << X << " " << Y << " " << Z << std::endl;

    if( X >= 2
     && X <  97
     && Y >= 2
     && Y <  97
     && Z >= 2
     && Z <  97
    )
    {

        ////std::cout << "in range" << std::endl;


        std::vector<Block*> points;
        std::vector<Block*> directed_points;
        for(int i=-2;i<=2;i++)
        for(int j=-2;j<=2;j++)
        for(int k=-2;k<=2;k++){
        Block* p = data[(int)X+i][(int)Y+j][(int)Z+k];
        if(p != NULL)
        if(p->type > 0)
        points.push_back(p);
        }
        directed_points = points;

        std::sort(points.begin(),points.end(),point_comparator);
        std::sort(directed_points.begin(),directed_points.end(),point_comparator_directional);
        if(directed_points.size() > 0)
        selection = directed_points[0];

        

        bool collision = false;

        double surface_vx = -(yPosition*asteroid_wz - zPosition*asteroid_wy);
        double surface_vy = -(zPosition*asteroid_wx - xPosition*asteroid_wz);
        double surface_vz = -(xPosition*asteroid_wy - yPosition*asteroid_wx);

        for(int i=0;i<points.size();i++)
        if(points[i] != NULL)
        if(points[i]->type > 0){
        ////std::cout << "collision" << std::endl;
        {
            double x = (xPosition) - ((points[i]->x-50)*asteroid_m[0]+(points[i]->y-50)*asteroid_m[4]+(points[i]->z-50)*asteroid_m[8]);
            double y = (yPosition) - ((points[i]->x-50)*asteroid_m[1]+(points[i]->y-50)*asteroid_m[5]+(points[i]->z-50)*asteroid_m[9]);
            double z = (zPosition) - ((points[i]->x-50)*asteroid_m[2]+(points[i]->y-50)*asteroid_m[6]+(points[i]->z-50)*asteroid_m[10]);
            double r = sqrt(x*x+y*y+z*z);
            ////std::cout << r << std::endl;
            if(r > 1e-4){
                x /= r;
                y /= r;
                z /= r;
            }
            if(points[i]->type != 29)
            if(r < 1.35){
                xPosition += (1.35-r)*x;
                yPosition += (1.35-r)*y;
                zPosition += (1.35-r)*z;
 
                xVelocity = .98*(xVelocity-surface_vx) + surface_vx;
                yVelocity = .98*(yVelocity-surface_vy) + surface_vy;
                zVelocity = .98*(zVelocity-surface_vz) + surface_vz;
 
            }

            if(points[i]->type != 29)
            if(r < 3.0){
                collision = true;
            } 

            //if(r < 1.67)
            if(data[points[i]->x][points[i]->y][points[i]->z] == selection){
            if(PLACE)
            {

                PLACE = false;

                

//                int __x = points[i]->x;
//                int __y = points[i]->y;
//                int __z = points[i]->z;

                bool continu = false;

                int m_x;
                int m_y;
                int m_z;
                double dist = 1000;
                double tmp_dist;

                for(int p_x=-1;p_x<=1;p_x++)
                for(int p_y=-1;p_y<=1;p_y++)
                for(int p_z=-1;p_z<=1;p_z++)
                if(p_x*p_x+p_y*p_y+p_z*p_z<=1)
                if(data[points[i]->x+p_x][points[i]->y+p_y][points[i]->z+p_z] == NULL){

                    double t_x = (xPosition-1.5*cameraRotation[2]+1.0*mouse_delta_y*cameraRotation[1]+1.0*mouse_delta_x*cameraRotation[0]) - ((points[i]->x+p_x-50)*asteroid_m[0]+(points[i]->y+p_y-50)*asteroid_m[4]+(points[i]->z+p_z-50)*asteroid_m[8]);
                    double t_y = (yPosition-1.5*cameraRotation[6]+1.0*mouse_delta_y*cameraRotation[5]+1.0*mouse_delta_x*cameraRotation[4]) - ((points[i]->x+p_x-50)*asteroid_m[1]+(points[i]->y+p_y-50)*asteroid_m[5]+(points[i]->z+p_z-50)*asteroid_m[9]);
                    double t_z = (zPosition-1.5*cameraRotation[10]+1.0*mouse_delta_y*cameraRotation[9]+1.0*mouse_delta_x*cameraRotation[8]) - ((points[i]->x+p_x-50)*asteroid_m[2]+(points[i]->y+p_y-50)*asteroid_m[6]+(points[i]->z+p_z-50)*asteroid_m[10]);
                    tmp_dist = sqrt(t_x*t_x+t_y*t_y+t_z*t_z);
 
                    if(tmp_dist < dist){
                        dist = tmp_dist;
                        m_x = p_x;
                        m_y = p_y;
                        m_z = p_z;
                    }

                    continu = true;

                }else
                if(data[points[i]->x+p_x][points[i]->y+p_y][points[i]->z+p_z]->type == 0){

                    double t_x = (xPosition-1.5*cameraRotation[2]+1.0*mouse_delta_y*cameraRotation[1]+1.0*mouse_delta_x*cameraRotation[0]) - ((points[i]->x+p_x-50)*asteroid_m[0]+(points[i]->y+p_y-50)*asteroid_m[4]+(points[i]->z+p_z-50)*asteroid_m[8]);
                    double t_y = (yPosition-1.5*cameraRotation[6]+1.0*mouse_delta_y*cameraRotation[5]+1.0*mouse_delta_x*cameraRotation[4]) - ((points[i]->x+p_x-50)*asteroid_m[1]+(points[i]->y+p_y-50)*asteroid_m[5]+(points[i]->z+p_z-50)*asteroid_m[9]);
                    double t_z = (zPosition-1.5*cameraRotation[10]+1.0*mouse_delta_y*cameraRotation[9]+1.0*mouse_delta_x*cameraRotation[8]) - ((points[i]->x+p_x-50)*asteroid_m[2]+(points[i]->y+p_y-50)*asteroid_m[6]+(points[i]->z+p_z-50)*asteroid_m[10]);
                    tmp_dist = sqrt(t_x*t_x+t_y*t_y+t_z*t_z);
 
                    if(tmp_dist < dist){
                        dist = tmp_dist;
                        m_x = p_x;
                        m_y = p_y;
                        m_z = p_z;
                    }

                    continu = true;

                }

                if(continu)
                if(player->tool.v.size()>0)
                {

                if(player->place_selection < 0)
                    player->place_selection += player->tool.v.size();
                int texture = (player->tool.v[(player->place_selection)%player->tool.v.size()]->texture);
                int type = player->tool.v[(player->place_selection)%player->tool.v.size()]->type;
                if(type == 24){
                    if(player->water < 99){    
                        player->tool.remove_item(type);
                        player->water = 100;
                    }
                    break;
                }
                if(type == 25){
                    if(player->food < 99){
                        player->tool.remove_item(type);
                        player->food = 100;
                    }
                    break;
                }
                player->tool.remove_item(type);
                if(data[points[i]->x+m_x][points[i]->y+m_y][points[i]->z+m_z])
                    delete_block(points[i]->x+m_x,points[i]->y+m_y,points[i]->z+m_z);
                Block* bl = add_block(points[i]->x+m_x,points[i]->y+m_y,points[i]->z+m_z,type);

                if(!SHIFT)
                {
                    mesh.AddBox( bl->x-50
                               , bl->y-50
                               , bl->z-50
                               , 0.5
                               , 0.5
                               , 0.5
                               , bl->type
                               , bl
                               );
                }
                else
                {
                    mesh.AddBox( bl->x-50
                               , bl->y-50
                               , bl->z-50
                               , 0.5
                               , 0.5
                               , 0.5
                               , bl->type
                               , bl
                               , 1.0
                               , true

                               , data[points[i]->x+1][points[i]->y  ][points[i]->z  ] != NULL || data[points[i]->x+1][points[i]->y  ][points[i]->z  ]->type == 0 
                               , data[points[i]->x-1][points[i]->y  ][points[i]->z  ] != NULL || data[points[i]->x-1][points[i]->y  ][points[i]->z  ]->type == 0
                               , data[points[i]->x  ][points[i]->y+1][points[i]->z  ] != NULL || data[points[i]->x  ][points[i]->y+1][points[i]->z  ]->type == 0
                               , data[points[i]->x  ][points[i]->y-1][points[i]->z  ] != NULL || data[points[i]->x  ][points[i]->y-1][points[i]->z  ]->type == 0
                               , data[points[i]->x  ][points[i]->y  ][points[i]->z+1] != NULL || data[points[i]->x  ][points[i]->y  ][points[i]->z+1]->type == 0
                               , data[points[i]->x  ][points[i]->y  ][points[i]->z-1] != NULL || data[points[i]->x  ][points[i]->y  ][points[i]->z-1]->type == 0

                               , data[points[i]->x+1][points[i]->y+1][points[i]->z+1] != NULL || data[points[i]->x+1][points[i]->y+1][points[i]->z+1]->type == 0
                               , data[points[i]->x+1][points[i]->y+1][points[i]->z-1] != NULL || data[points[i]->x+1][points[i]->y+1][points[i]->z-1]->type == 0
                               , data[points[i]->x+1][points[i]->y-1][points[i]->z+1] != NULL || data[points[i]->x+1][points[i]->y-1][points[i]->z+1]->type == 0
                               , data[points[i]->x+1][points[i]->y-1][points[i]->z-1] != NULL || data[points[i]->x+1][points[i]->y-1][points[i]->z-1]->type == 0
                               , data[points[i]->x-1][points[i]->y+1][points[i]->z+1] != NULL || data[points[i]->x-1][points[i]->y+1][points[i]->z+1]->type == 0
                               , data[points[i]->x-1][points[i]->y+1][points[i]->z-1] != NULL || data[points[i]->x-1][points[i]->y+1][points[i]->z-1]->type == 0
                               , data[points[i]->x-1][points[i]->y-1][points[i]->z+1] != NULL || data[points[i]->x-1][points[i]->y-1][points[i]->z+1]->type == 0
                               , data[points[i]->x-1][points[i]->y-1][points[i]->z-1] != NULL || data[points[i]->x-1][points[i]->y-1][points[i]->z-1]->type == 0

                               , data[points[i]->x+1][points[i]->y+1][points[i]->z  ] != NULL || data[points[i]->x+1][points[i]->y+1][points[i]->z  ]->type == 0
                               , data[points[i]->x+1][points[i]->y-1][points[i]->z  ] != NULL || data[points[i]->x+1][points[i]->y-1][points[i]->z  ]->type == 0
                               , data[points[i]->x-1][points[i]->y+1][points[i]->z  ] != NULL || data[points[i]->x-1][points[i]->y+1][points[i]->z  ]->type == 0
                               , data[points[i]->x-1][points[i]->y-1][points[i]->z  ] != NULL || data[points[i]->x-1][points[i]->y-1][points[i]->z  ]->type == 0
                               , data[points[i]->x+1][points[i]->y  ][points[i]->z+1] != NULL || data[points[i]->x+1][points[i]->y  ][points[i]->z+1]->type == 0
                               , data[points[i]->x+1][points[i]->y  ][points[i]->z-1] != NULL || data[points[i]->x+1][points[i]->y  ][points[i]->z-1]->type == 0
                               , data[points[i]->x-1][points[i]->y  ][points[i]->z+1] != NULL || data[points[i]->x-1][points[i]->y  ][points[i]->z+1]->type == 0
                               , data[points[i]->x-1][points[i]->y  ][points[i]->z-1] != NULL || data[points[i]->x-1][points[i]->y  ][points[i]->z-1]->type == 0
                               , data[points[i]->x  ][points[i]->y+1][points[i]->z+1] != NULL || data[points[i]->x  ][points[i]->y+1][points[i]->z+1]->type == 0
                               , data[points[i]->x  ][points[i]->y+1][points[i]->z-1] != NULL || data[points[i]->x  ][points[i]->y+1][points[i]->z-1]->type == 0
                               , data[points[i]->x  ][points[i]->y-1][points[i]->z+1] != NULL || data[points[i]->x  ][points[i]->y-1][points[i]->z+1]->type == 0
                               , data[points[i]->x  ][points[i]->y-1][points[i]->z-1] != NULL || data[points[i]->x  ][points[i]->y-1][points[i]->z-1]->type == 0

                               );
                }

                }

/*
                int damage = 10;
                if( data[points[i]->x][points[i]->y][points[i]->z]->armor < damage)
                data[points[i]->x][points[i]->y][points[i]->z]->health -= damage - data[points[i]->x][points[i]->y][points[i]->z]->armor;
                if(data[points[i]->x][points[i]->y][points[i]->z]->health < 0){

                mesh.RemoveBox( data[points[i]->x][points[i]->y][points[i]->z] );
                delete_block( points[i]->x
                            , points[i]->y
                            , points[i]->z
                            );
                }
*/
/* 
                for(int _x=-1;_x<=1;_x+=2)
                for(int _y=-1;_y<=1;_y+=2)
                for(int _z=-1;_z<=1;_z+=2){
                Block* block = data[__x+_x][__y+_y][__z+_z];
                if(block != NULL)
                if(block->visible == false)
                mesh.AddBox( block->x-50
                           , block->y-50
                           , block->z-50
                           , 0.5
                           , 0.5
                           , 0.5
                           , block->type
                           , block
                           );
                }
*/


//                cmesh->LoadMesh(&mesh);
//                cmesh->UpdateVBOs();

                update_scene = true;

            }


            }

        }
        }

        {
        player->air = 0;
        Block* p = data[(int)X][(int)Y][(int)Z];
        if(p!=NULL)
        if(p->air > 0.01){
            player->air = p->air;
        if(p->air > 5.0){
            double x = (xPosition) - ((p->x-50)*asteroid_m[0]+(p->y-50)*asteroid_m[4]+(p->z-50)*asteroid_m[8]);
            double y = (yPosition) - ((p->x-50)*asteroid_m[1]+(p->y-50)*asteroid_m[5]+(p->z-50)*asteroid_m[9]);
            double z = (zPosition) - ((p->x-50)*asteroid_m[2]+(p->y-50)*asteroid_m[6]+(p->z-50)*asteroid_m[10]);
            double r = sqrt(x*x+y*y+z*z);
            if(r < 5.0)
                player->oxygen = 100;
        }
        }
        }
        if(collision){

            double vel = sqrt( (xVelocity-surface_vx)*(xVelocity-surface_vx) 
                             + (yVelocity-surface_vy)*(yVelocity-surface_vy)
                             + (zVelocity-surface_vz)*(zVelocity-surface_vz)
                             );
            if(forced == false)
            if(vel < .1) 
            {
                xVelocity = surface_vx;
                yVelocity = surface_vy;
                zVelocity = surface_vz;
            }

            if(vel > 5.0) 
            {
                zVelocity = surface_vx + 5.0*(xVelocity-surface_vx)/vel;
                zVelocity = surface_vy + 5.0*(yVelocity-surface_vy)/vel;
                zVelocity = surface_vz + 5.0*(zVelocity-surface_vz)/vel;
            }

            rotateMatrixAroundVector( cameraRotation[0], cameraRotation[4], cameraRotation[8]
                                    , cameraRotation[1], cameraRotation[5], cameraRotation[9]
                                    , cameraRotation[2], cameraRotation[6], cameraRotation[10]
                                    , -(asteroid_wx*cameraRotation[0]+asteroid_wy*cameraRotation[4]+asteroid_wz*cameraRotation[8])
                                    , -(asteroid_wx*cameraRotation[1]+asteroid_wy*cameraRotation[5]+asteroid_wz*cameraRotation[9])
                                    , -(asteroid_wx*cameraRotation[2]+asteroid_wy*cameraRotation[6]+asteroid_wz*cameraRotation[10])
                                    );


        }
 

    }

    xPosition += xVelocity;
    yPosition += yVelocity;
    zPosition += zVelocity;

    double dx = mouse_x - _WIDTH/2;
    double dy = mouse_y - _HEIGHT/2;
    mouse_r = sqrt(dx*dx + dy*dy);

        mouse_delta_x = -(mouse_x-_WIDTH/2)/(WIDTH/2);
        mouse_delta_y = -(mouse_y-_HEIGHT/2)/(WIDTH/2);

    if(!MENU)
    if(mouse_r > 150.0) {
//        yRotated += (mouse_x - _WIDTH /2)/1000.0;
//        xRotated += (mouse_y - _HEIGHT/2)/1000.0;

        rotateMatrixAroundVector( cameraRotation[0], cameraRotation[4], cameraRotation[8]
                                , cameraRotation[1], cameraRotation[5], cameraRotation[9]
                                , cameraRotation[2], cameraRotation[6], cameraRotation[10]
                                , 0, (mouse_x - _WIDTH/2)*0.00008, 0
                                );


        rotateMatrixAroundVector( cameraRotation[0], cameraRotation[4], cameraRotation[8]
                                , cameraRotation[1], cameraRotation[5], cameraRotation[9]
                                , cameraRotation[2], cameraRotation[6], cameraRotation[10]
                                , (mouse_y - _HEIGHT/2)*0.00008, 0, 0
                                );


    }

    rotateMatrixAroundVector( asteroid_m[0], asteroid_m[4], asteroid_m[8]
                            , asteroid_m[1], asteroid_m[5], asteroid_m[9]
                            , asteroid_m[2], asteroid_m[6], asteroid_m[10]
                            , asteroid_wx, asteroid_wy, asteroid_wz
                            );


//    //std::cout << yRotated << " " << xRotated << std::endl;

    player->update();

    glutPostRedisplay();
}


void reshape(int x, int y)
{
    if (y == 0 || x == 0) return;  //Nothing is visible then, so return
    //Set a new projection matrix
    glMatrixMode(GL_PROJECTION);  
    glLoadIdentity();
    //Angle of view:40 degrees
    //Near clipping plane distance: 0.5f
    //Far clipping plane distance: 20.0
     
    gluPerspective(40.0,(GLdouble)x/(GLdouble)y,0.5f,200000.0);
    glMatrixMode(GL_MODELVIEW);
    glViewport(0,0,x,y);  //Use the whole window for rendering
}

void processSpecialKeys(int key, int x, int y) {

    int mod;
    mod = glutGetModifiers();
    if (mod==GLUT_ACTIVE_SHIFT) {
        SHIFT = true;
    }else{
        SHIFT = false;
    }

}

void keyboard_press(unsigned char key, int x, int y)
{

    int mod;
    mod = glutGetModifiers();
    if (mod==GLUT_ACTIVE_SHIFT) {
        SHIFT = true;
    }else{
        SHIFT = false;
    }

    if(!TEXT){
        switch(key) {
            case '1':
                tool_selection = MINING_TOOL_TYPE;
                break;
            case '2':
                tool_selection = WEAPON_TYPE;
                break;
            case 'p':
                RECORD = !RECORD;
                break;
            case 27:
                if( !MENU )
                exit(0);
                break;
            case 'w':
                if( !MENU )
                FORWARD = true;
                else
                CONTROL_FORWARD = true;
                break;
            case 's':
                if( !MENU )
                BACKWARD = true;
                else
                CONTROL_BACKWARD = true;
                break;
            case 'a':
                if( !MENU )
                LEFT = true;
                else
                CONTROL_LEFT = true;
                break;
            case 'd':
                if( !MENU )
                RIGHT = true;
                else
                CONTROL_RIGHT = true;
                break;
            case 'q':
                if( !MENU )
                UP = true;
                else
                CONTROL_UP = true;
                break;
            case 'z':
                if( !MENU )
                DOWN = true;
                else
                CONTROL_DOWN = true;
                break;
            case 't':
                player->place_selection++;
                break;
            case 'g':
                player->place_selection--;
                break;
            case 'e':
                if(selection || MENU)
                if(selection->menu || MENU)
                MENU = !MENU;
                break;
        }
    }else{
        //std::cout << "key=" << (unsigned int)key << std::endl;
        if(key == 8){
            if(string_ptr->size() > 0)
                string_ptr->resize(string_ptr->size()-1);
        }
        if(key == 13){
            TEXT = false;
            NUMERIC_INPUT = false;
            return;
        }else{
            if(NUMERIC_INPUT){
                if(key < 48 || key > 57)
                    return;
            }
            *string_ptr += (unsigned char)key;
        }
    }


}

void keyboard_release(unsigned char key, int x, int y)
{


    switch(key) {
        case 'w':
            FORWARD = false;
            CONTROL_FORWARD = false;
            break;
        case 's':
            BACKWARD = false;
            CONTROL_BACKWARD = false;
            break;
        case 'a':
            LEFT = false;
            CONTROL_LEFT = false;
            break;
        case 'd':
            RIGHT = false;
            CONTROL_RIGHT = false;
            break;
        case 'q':
            UP = false;
            CONTROL_UP = false;
            break;
        case 'z':
            DOWN = false;
            CONTROL_DOWN = false;
            break;

    }


}


void mouse_drag_motion(int x, int y)
{

    mouse_x = x;
    mouse_y = y;


}


void mouse_motion(int x, int y)
{

    mouse_x = x;
    mouse_y = y;


}

void mouse(int button, int state, int x, int y)
{

    int mod;
    mod = glutGetModifiers();
    if (mod==GLUT_ACTIVE_SHIFT) {
        SHIFT = true;
    }else{
        SHIFT = false;
    }


    if(button == GLUT_LEFT_BUTTON)
        if(state == GLUT_DOWN)
            MINE = true;
        else
        if(state == GLUT_UP)
            MINE = false;

    if(button == GLUT_RIGHT_BUTTON)
        if(state == GLUT_DOWN)
            PLACE = true;
        else
        if(state == GLUT_UP)
            PLACE = false;

    if (button == 3)
    {
        if(state == GLUT_UP)
            return;
        player->place_selection++;
    }

    if (button == 4)
    {
        if(state == GLUT_UP)
            return;
        player->place_selection--;
    }

}


void init(void)
{
	glShadeModel(GL_SMOOTH);					// Enable Smooth Shading

    glClearColor( 0.0, 0.0, 0.0, 0.0 );

    glClearDepth( 1.0f );
    glDepthFunc( GL_LESS );
    glEnable( GL_DEPTH_TEST );

    glEnable( GL_CULL_FACE );

	glDisable(GL_DEPTH_TEST);					// Disable Depth Testing
	//glEnable(GL_BLEND);						// Enable Blending
	glBlendFunc(GL_SRC_ALPHA,GL_ONE);				// Type Of Blending To Perform
	glHint(GL_PERSPECTIVE_CORRECTION_HINT,GL_NICEST);		// Really Nice Perspective Calculations
	glHint(GL_POINT_SMOOTH_HINT,GL_NICEST);				// Really Nice Point Smoothing

    glEnable( GL_TEXTURE_2D );


    //glBlendFunc( GL_SRC_ALPHA, GL_ONE );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

}

GLuint LoadTextureRAW( const char * filename, int wrap, int size )
{
    GLuint texture;
    int width, height;
    unsigned int * data;
    FILE * file;

    // open texture data
    file = fopen( filename, "rb" );
    if ( file == NULL ) return 0;

    // allocate buffer
    width = size;
    height = size;
    data = (unsigned int*)malloc( width * height * 3 );

    // read texture data
    int ret = fread( data, width * height * 3, 1, file );
    fclose( file );

    // allocate a texture name
    glGenTextures( 1, &texture );

    // select our current texture
    glBindTexture( GL_TEXTURE_2D, texture );

    // select modulate to mix texture with color for shading
    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

    // when texture area is small, bilinear filter the closest mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                     GL_LINEAR_MIPMAP_NEAREST );
    // when texture area is large, bilinear filter the first mipmap
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    // if wrap is true, the texture wraps over at the edges (repeat)
    //       ... false, the texture ends at the edges (clamp)
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S,
                     wrap ? GL_REPEAT : GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,
                     wrap ? GL_REPEAT : GL_CLAMP );

    // build our texture mipmaps
    gluBuild2DMipmaps( GL_TEXTURE_2D, 3, width, height,
                       GL_RGB, GL_UNSIGNED_BYTE, data );

    // free buffer
    free( data );

    return texture;
}

GLuint GenerateSphere(float radius = 1.0){
  
  GLUquadricObj *sphere=NULL;
  sphere = gluNewQuadric();
  gluQuadricDrawStyle(sphere, GLU_FILL);
  gluQuadricTexture(sphere, true);
  gluQuadricNormals(sphere, GLU_SMOOTH);
  //Making a display list
  GLuint mysphereID = glGenLists(1);
  glNewList(mysphereID, GL_COMPILE);
  gluSphere(sphere, radius, 40, 40);
  glEndList();
  //gluDeleteQuadric(sphere);
  return mysphereID;

}

int main(int argc, char** argv){
    srand(time(0));

    cameraRotation[0] = 1;
    cameraRotation[1] = 0;
    cameraRotation[2] = 0;
    cameraRotation[3] = 0;

    cameraRotation[4] = 0;
    cameraRotation[5] = 1;
    cameraRotation[6] = 0;
    cameraRotation[7] = 0;

    cameraRotation[8] = 0;
    cameraRotation[9] = 0;
    cameraRotation[10]= 1;
    cameraRotation[11]= 0;

    cameraRotation[12]= 0;
    cameraRotation[13]= 0;
    cameraRotation[14]= 0;
    cameraRotation[15]= 1;

    asteroid_m[0] = 1;
    asteroid_m[1] = 0;
    asteroid_m[2] = 0;
    asteroid_m[3] = 0;

    asteroid_m[4] = 0;
    asteroid_m[5] = 1;
    asteroid_m[6] = 0;
    asteroid_m[7] = 0;

    asteroid_m[8] = 0;
    asteroid_m[9] = 0;
    asteroid_m[10]= 1;
    asteroid_m[11]= 0;

    asteroid_m[12]= 0;
    asteroid_m[13]= 0;
    asteroid_m[14]= 0;
    asteroid_m[15]= 1;





    glutInit(&argc, argv);
    //we initizlilze the glut. functions
    //glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
    //glutInitDisplayMode(GLUT_SINGLE|GLUT_RGBA);
    glutInitWindowSize(WIDTH,HEIGHT);
    //glutInitWindowPosition((_WIDTH-WIDTH)/2, (_HEIGHT-HEIGHT)/2);
    glutInitWindowPosition( 0, 0 );
    glutCreateWindow(argv[0]);
GLenum err = glewInit();
if (GLEW_OK != err)
{
  /*  Problem: glewInit failed, something is seriously wrong. */
  fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
}
fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION)); 

    glutFullScreen();



    GLint m_viewport[4];

    glGetIntegerv( GL_VIEWPORT, m_viewport );

    std::cout << "window:" << std::endl;
    std::cout << m_viewport[0] << std::endl;
    std::cout << m_viewport[1] << std::endl;
    std::cout << m_viewport[2] << std::endl;
    std::cout << m_viewport[3] << std::endl;


    init();
    init_blocks();
   
    item_array.push_back( new Item( 0, 0, "null"                  , 0 ) );

    item_array.push_back( new Item( 1 // int _type
                                  , 1 // int _volume
                                  , "sand" // string _name
                                  , 1 // int _texture
                                  )
                        );
   
    item_array.push_back( new Item( 2, 1, "rock"                  , 2 ) );
    item_array.push_back( new Item( 3, 1, "iron"                  , 3 ) );
    item_array.push_back( new Item( 4, 1, "ice"                   , 4 ) );
    item_array.push_back( new Item( 5, 1, "uranium"               , 5 ) );
    item_array.push_back( new Item( 6, 1, "carbon"                , 6 ) );
    item_array.push_back( new Item( 7, 1, "hydrocarbon"           , 7 ) );

    item_array.push_back( new Item( 8, 5, "solar panel"           , 8 ) );
    item_array.push_back( new Item( 9, 2, "wire"                  , 9 ) );
    item_array.push_back( new Item(10,10, "capacitor"             ,10 ) );
    item_array.push_back( new Item(11, 5, "lamp"                  ,11 ) );

    item_array.push_back( new Item(12,10, "3D printer"            ,12 ) );
    item_array.push_back( new Item(13,10, "atmosphere generator"  ,13 ) );
    item_array.push_back( new Item(14,10, "container"             ,14 ) );
    item_array.push_back( new Item(15,10, "refinery"              ,15 ) );

    item_array.push_back( new Item(16,10, "engine"                ,16 ) );
    item_array.push_back( new Item(17,10, "control panel"         ,17 ) );
    item_array.push_back( new Item(18,10, "camera"                ,18 ) );
    item_array.push_back( new Item(19,10, "heat dissipator"       ,19 ) );

    item_array.push_back( new Item(20, 2, "stone"                 ,20 ) );
    item_array.push_back( new Item(21, 2, "metal"                 ,21 ) );
    item_array.push_back( new Item(22, 2, "ceramic"               ,22 ) );
    item_array.push_back( new Item(23, 2, "carbon fiber"          ,23 ) );

    item_array.push_back( new Item(24, 1, "water"                 ,24 ) );
    item_array.push_back( new Item(25, 1, "food"                  ,25 ) );
    item_array.push_back( new Item(26,10, "condenser"             ,26 ) );
    item_array.push_back( new Item(27,10, "greenhouse"            ,27 ) );

    item_array.push_back( new Item(28, 1, "window"                ,28 ) );
    item_array.push_back( new Item(29,10, "door"                  ,29 ) );
    item_array.push_back( new Item(30,50, "nuclear generator"     ,30 ) );
    item_array.push_back( new Item(31,10, "heat dissipator"       ,31 ) );

    enemy = std::vector<Enemy*>(10);
    for(int i=0;i<5;i++){
        float x = 1.0-2.0*((rand()%1000)/1000.0);
        float y = 1.0-2.0*((rand()%1000)/1000.0);
        float z = 1.0-2.0*((rand()%1000)/1000.0);
        float r = sqrt(x*x+y*y+z*z);
        x /= r;
        y /= r;
        z /= r;
        x *= 20.0;
        y *= 20.0;
        z *= 20.0;
        enemy[i] = new CreatureEnemy(x,y,z);
    }
    for(int i=0;i<5;i++)
        enemy[i+5] = new FighterEnemy(400+rand()%10,400+rand()%10,400+rand()%10);
        //enemy[i+5] = new FighterEnemy(40+rand()%10,40+rand()%10,40+rand()%10);

    resource_projectile = std::vector<Projectile*>(10000);
    for(int i=0;i<10000;i++)
        resource_projectile[i] = new ResourceProjectile();

    mining_projectile = std::vector<Projectile*>(10000);
    for(int i=0;i<10000;i++)
        mining_projectile[i] = new MiningProjectile();


    projectile = std::vector<Projectile*>(10000);
    for(int i=0;i<10000;i++)
        projectile[i] = new WeaponProjectile();

    player = new Player(100,1000);

    player->tool.insert_item(8);
    player->tool.insert_item(9);
    player->tool.insert_item(12);
    player->tool.insert_item(14);


    for(int i=0;i<100;i++)
    for(int j=0;j<100;j++)
    for(int k=0;k<100;k++){
    double r = sqrt(pow(i-50,2)+pow(j-50,2)+pow(k-50,2));
    if(r<33&&fabs(r-20)>3+rand()%2) {
    int level = r/6.15 + rand()%2;
    if(level > 7)
        level = 7;
    level = 8-level;
    if(level==5)level=7;
    else if(level==7)level=5;
    add_block(i,j,k,level);
    }
    }

    for(int i=0;i<blocks.size();i++){

        int x = blocks[i]->x;
        int y = blocks[i]->y;
        int z = blocks[i]->z;

        bool draw = false;

        if(x == 0 )
            draw = true;
        if(y == 0 )
            draw = true;
        if(z == 0 )
            draw = true;
        if(x == 99)
            draw = true;
        if(y == 99)
            draw = true;
        if(z == 99)
            draw = true;

        if(draw == false){
            draw = (data[x+1][y][z] == NULL)
                || (data[x-1][y][z] == NULL)
                || (data[x][y+1][z] == NULL)
                || (data[x][y-1][z] == NULL)
                || (data[x][y][z+1] == NULL)
                || (data[x][y][z-1] == NULL)

                || (data[x+1][y+1][z+1] == NULL)
                || (data[x-1][y+1][z+1] == NULL)
                || (data[x+1][y-1][z+1] == NULL)
                || (data[x-1][y-1][z+1] == NULL)
                || (data[x+1][y+1][z-1] == NULL)
                || (data[x-1][y+1][z-1] == NULL)
                || (data[x+1][y-1][z-1] == NULL)
                || (data[x-1][y-1][z-1] == NULL)

                || (data[x+1][y+1][z] == NULL)
                || (data[x-1][y+1][z] == NULL)
                || (data[x+1][y-1][z] == NULL)
                || (data[x-1][y-1][z] == NULL)

                || (data[x+1][y][z+1] == NULL)
                || (data[x-1][y][z+1] == NULL)
                || (data[x+1][y][z-1] == NULL)
                || (data[x-1][y][z-1] == NULL)

                || (data[x][y+1][z+1] == NULL)
                || (data[x][y-1][z+1] == NULL)
                || (data[x][y+1][z-1] == NULL)
                || (data[x][y-1][z-1] == NULL)

                 ;
        }


        if(draw) {


            mesh.AddBox( blocks[i]->x-50
                       , blocks[i]->y-50
                       , blocks[i]->z-50
                       , 0.5
                       , 0.5
                       , 0.5
                       , blocks[i]->type
                       , blocks[i]
                       , 1.0
                       , true

                       , data[x+1][y][z] != NULL
                       , data[x-1][y][z] != NULL
                       , data[x][y+1][z] != NULL
                       , data[x][y-1][z] != NULL
                       , data[x][y][z+1] != NULL
                       , data[x][y][z-1] != NULL

                       , data[x+1][y+1][z+1] != NULL
                       , data[x+1][y+1][z-1] != NULL
                       , data[x+1][y-1][z+1] != NULL
                       , data[x+1][y-1][z-1] != NULL
                       , data[x-1][y+1][z+1] != NULL
                       , data[x-1][y+1][z-1] != NULL
                       , data[x-1][y-1][z+1] != NULL
                       , data[x-1][y-1][z-1] != NULL

                       , data[x+1][y+1][z] != NULL
                       , data[x+1][y-1][z] != NULL
                       , data[x-1][y+1][z] != NULL
                       , data[x-1][y-1][z] != NULL
                       , data[x+1][y][z+1] != NULL
                       , data[x+1][y][z-1] != NULL
                       , data[x-1][y][z+1] != NULL
                       , data[x-1][y][z-1] != NULL
                       , data[x][y+1][z+1] != NULL
                       , data[x][y+1][z-1] != NULL
                       , data[x][y-1][z+1] != NULL
                       , data[x][y-1][z-1] != NULL

                       );


        }

    }
 
    //MeshSmoother mesh_smoother;

    //mesh_smoother.LoadMesh(&mesh);

    //for(int i=0;i<3;i++)
    //{
    //    mesh_smoother.Smooth(&mesh,.2);
    //}

    cmesh = new CMesh();

    cmesh->LoadMesh(&mesh);

    cmesh->BuildVBOs();

    for(int i=0;i<3200;i++)
    {
        float x = (rand()%1000/500.0-1.0);
        float y = (rand()%1000/500.0-1.0);
        float z = (rand()%1000/500.0-1.0);
        float r = sqrt(x*x+y*y+z*z);
        stars.push_back(new Block( 10000.0*x/r
                                 , 10000.0*y/r
                                 , 10000.0*z/r
                                 , 1
                                 )
                       );
    }
    
    //add_block(3,3,3,1);

    texture[0] = LoadTextureRAW("texture2.raw",0,8*512);
    texture[1] = LoadTextureRAW("sun.raw",1,512);
    texture[2] = LoadTextureRAW("foo_fighter.raw",1,512);
    texture[3] = LoadTextureRAW("Particle.raw",1,32);
    CloudGenerator * cloud_generator = new CloudGenerator();
    cloud_generator->generateTexture();
    texture[4] = cloud_generator->texName;
    texture[5] = LoadTextureRAW("Jupiter.raw",1,512);
    texture[7] = LoadTextureRAW("Io.raw",1,512);
    texture[8] = LoadTextureRAW("Europa.raw",1,512);
    texture[9] = LoadTextureRAW("Ganymede.raw",1,512);
    texture[10]= LoadTextureRAW("Callisto.raw",1,512);

    pthread_t thread;
    int rc = pthread_create(&thread,NULL,CalculateJupiterTexture,(void*)&Jupiter_cloud_generator);

    space_station_mesh.read("Zenith_triangulated.obj");

    space_station_cmesh = new CMesh();

    space_station_cmesh->LoadMesh(&space_station_mesh);

    space_station_cmesh->BuildVBOs();

    space_station_cmesh->UpdateVBOs();


//    enemy_ship_mesh.read("Player.obj",.1);
//    enemy_ship_mesh.read("EnemySpaceShip.obj",.01);
    enemy_drone_mesh.read("FighterDrone.obj",.01);

    enemy_drone_cmesh = new CMesh();

    enemy_drone_cmesh->LoadMesh(&enemy_drone_mesh);

    enemy_drone_cmesh->BuildVBOs();

    enemy_drone_cmesh->UpdateVBOs();


    enemy_creature_mesh.read("Creature.obj",.01);

    enemy_creature_cmesh = new CMesh();

    enemy_creature_cmesh->LoadMesh(&enemy_creature_mesh);

    enemy_creature_cmesh->BuildVBOs();

    enemy_creature_cmesh->UpdateVBOs();

    skybox = GenerateSphere();
    enemy_sphere = GenerateSphere(.25);
    projectile_sphere = GenerateSphere(.15);

    sun_generator = new SunGenerator();

    //sun_generator->LoadGLTextures();
    sun_generator->initialize();


    glutSetCursor(GLUT_CURSOR_NONE); 
    glutKeyboardUpFunc(keyboard_release);
    glutKeyboardFunc(keyboard_press);
    glutPassiveMotionFunc(mouse_motion);
    glutMouseFunc(mouse);
    glutMotionFunc(mouse_drag_motion);
    glutDisplayFunc(DrawCube);
    glutSpecialFunc(processSpecialKeys);
    glutReshapeFunc(reshape);
    //Set the function for the animation.
    glutIdleFunc(animation);
    glutMainLoop();
    pthread_exit(NULL);
    return 0;

}
 
