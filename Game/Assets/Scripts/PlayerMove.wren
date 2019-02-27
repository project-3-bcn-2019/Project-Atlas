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


//Attack
attack_duration {__attack_duration}
attack_duration=(v) {__attack_duration = v}

attack_current_time {__attack_current_time}
attack_current_time=(v) {__attack_current_time = v}

attacking  {__attacking}
attacking=(v) {__attacking = v}

 Start() {
direction = Vec3.zero()
dashing = false
dash_current_time = 0.0
attacking = false
}

 Update() {
var move = false
//TODO: Update the player's direction with the input and then move him using the speed
//TODO 2:If the player dashes, deactivate movement and make him dash
//Needed: A way to use classes effectively. comunication between scripts

if(dashing == false && attacking == false){

        var z_value = InputComunicator.getAxisNormalized(0,InputComunicator.L_AXIS_Y)
        direction.z = -z_value

        var x_value = InputComunicator.getAxisNormalized(0,InputComunicator.L_AXIS_X)
        direction.x = -x_value

         if(direction.z != 0 || direction.x != 0){
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

if(InputComunicator.getButton(0,InputComunicator.C_A, InputComunicator.KEY_DOWN) && dash_available && !attacking){
  dashing = true
  dash_current_time = 0.0
  move = false
}

if(InputComunicator.getButton(0,InputComunicator.C_X, InputComunicator.KEY_DOWN) && !attacking && !dashing){
  attacking = true
  attack_current_time = 0.0
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

if(attacking){
EngineComunicator.consoleOutput("Attacking")
 attack_current_time =  attack_current_time + Time.C_GetDeltaTime()
  if(attack_current_time >= attack_duration){
     attacking = false
  }
}
}
}