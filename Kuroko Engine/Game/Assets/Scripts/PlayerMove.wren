import "ObjectLinker" for ObjectLinker,
EngineComunicator,
InputComunicator,
Vec3,
Time

//For each var you declare, remember to create
//		setters [varname=(v) { __varname = v }]
//		and getters [varname { __varname }]
//The construct method is mandatory, do not erase!
//The import statement at the top og the cript is mandatory, do not erase!
//Be careful not to overwrite the methods declared in Game/ScriptingAPI/ObjectLinker.wren
//[gameObject] is a reserved identifier for the engine, don't use it for your own variables

class PlayerMove is ObjectLinker{

construct new(){}

//Movement
speed {__speed}
speed=(v){__speed = v}


direction {__direction}
direction=(v){__direction = v}

//Dash
dash_speed {__dash_speed}
dash_speed=(v){__dash_speed = v}

dash_duration {__dash_duration}
dash_duration=(v){__dash_duration = v}

dash_current_time {__dash_current_time}
dash_current_time=(v){__dash_current_time = v}

dash_cooldown {__dash_cooldown}
dash_cooldown=(v){__dash_cooldown = v}

dashing {__dashing}
dashing=(v){__dashing = v}

dash_available {__dash_available}
dash_available=(v){__dash_available = v}

class state{
no_state={0}
idle={1}
running={2}
dashing={3}
attack={4}

state {__state}
state=(v){__state = v}

construct new(){
  state = 0
}

construct state(var int){
  state = int
}

}

 Start() {
direction = Vec3.zero()
dashing = false
dash_current_time = 0.0
}

 Update() {
var move = false
//TODO: Update the player's direction with the input and then move him using the speed
//TODO 2:If the player dashes, deactivate movement and make him dash
//Needed: A way to use classes effectively. comunication between scripts

if(dashing == false){

if(InputComunicator.getButton(1, InputComunicator.L_AXIS_UP, InputComunicator.KEY_REPEAT)){
			direction.z = 1
			move = true
		}
		
if(InputComunicator.getButton(1, InputComunicator.L_AXIS_DOWN, InputComunicator.KEY_REPEAT)){
			direction.z = -1
			move = true
		}
if(InputComunicator.getButton(1, InputComunicator.L_AXIS_LEFT, InputComunicator.KEY_REPEAT)){
			direction.x = 1
            move = true
		}
		
if(InputComunicator.getButton(1, InputComunicator.L_AXIS_RIGHT, InputComunicator.KEY_REPEAT)){
			direction.z = -1
			move = true
		}


if(InputComunicator.getKey(InputComunicator.UP, InputComunicator.KEY_REPEAT)){
			direction.z = 1
            move = true
		}
		
		if(InputComunicator.getKey(InputComunicator.DOWN, InputComunicator.KEY_REPEAT)){
			direction.z = -1
            move = true
		}

		if(InputComunicator.getKey(InputComunicator.LEFT, InputComunicator.KEY_REPEAT)){
			direction.x = 1
            move = true
		}
		
		if(InputComunicator.getKey(InputComunicator.RIGHT, InputComunicator.KEY_REPEAT)){
			direction.x = -1
            move = true
		}

}

if(InputComunicator.getKey(InputComunicator.SPACE, InputComunicator.KEY_DOWN) && dash_available){
dashing = true
dash_current_time = 0.0
move = false
}

if(move){
  var movement = Vec3.new(direction.x*speed,0,direction.z*speed)
  modPos(movement.x,movement.y,movement.z)

  var pos = getPos("global")
  var look = Vec3.new(direction.x+pos.x,0,direction.z+pos.z)

  lookAt(look.x,0,look.z)
  direction = Vec3.zero()
}

if(dashing){
  dash_current_time =  dash_current_time + Time.C_GetDeltaTime()
  if(dash_current_time >= dash_duration){
     dashing = false
     dash_available = false
     dash_current_time = 0.0
  }
 var movement = Vec3.new(direction.x*dash_speed,0,direction.z*dash_speed)
 modPos(movement.x,movement.y,movement.z)
}
if(dash_available == false){
 dash_current_time =  dash_current_time + Time.C_GetDeltaTime()
 if(dash_current_time >= dash_cooldown){
     dash_available = true
  }
}
}
}