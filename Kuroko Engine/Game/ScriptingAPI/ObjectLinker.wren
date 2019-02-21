
class ObjectComunicator{
	foreign static C_setPos(gameObject, x, y, z)
	foreign static C_modPos(gameObject, x, y, z)
	foreign static C_lookAt(gameObject, x, y, z)

	foreign static C_getPosX(gameObject, mode)
	foreign static C_getPosY(gameObject, mode)
	foreign static C_getPosZ(gameObject, mode)

	foreign static C_getPitch(gameObject)
	foreign static C_getYaw(gameObject)
	foreign static C_getRoll(gameObject)

	foreign static C_Kill(gameObject)
	foreign static C_MoveForward(gameObject, speed)
}

class Math{
	foreign static C_sqrt(number)

	static pow(number, power){
		var ret = 1
		for(i in 0...power){
			ret = ret * number
		}
		return ret
	}
}

class Time{
	foreign static C_GetDeltaTime()

}

class Vec3{

	construct zero(){
		x = 0
		y = 0
		z = 0
	}

	construct new(n_x, n_y, n_z){
		x = n_x
		y = n_y
		z = n_z
	}

	x {_x}
	x=(v) {_x = v}

	y {_y}
	y=(v) {_y = v}

	z {_z}
	z=(v) {_z = v}

	magnitude {Math.C_sqrt(Math.pow(x, 2) + Math.pow(y, 2) + Math.pow(z, 2))}

}

class EngineComunicator{
	foreign static consoleOutput(message)
	foreign static Instantiate(prefab_name, x, y, z, pitch, yaw, roll)
	foreign static getTime()
	foreign static BreakPoint(message, variable, variable_name)

	static Instantiate(prefab_name, pos, euler){
		EngineComunicator.Instantiate(prefab_name, pos.x, pos.y, pos.z, euler.x, euler.y, euler.z)
	}
}

class InputComunicator{
	
	static UP {82}
	static DOWN {81}
	static LEFT {80}
	static RIGHT {79}
	static SPACE {44}

	static KEY_DOWN {1}
	static KEY_REPEAT {2}
	foreign static getKey(key, mode)
	foreign static getMouseRaycastX()
	foreign static getMouseRaycastY()
	foreign static getMouseRaycastZ()
}

class ObjectLinker{
	gameObject { _gameObject}		// UUID of the linked GO
	gameObject=(v){ _gameObject = v}

	setPos(x,y,z){
		ObjectComunicator.C_setPos(gameObject, x, y, z)
	}
	modPos(x,y,z){
		ObjectComunicator.C_modPos(gameObject, x, y, z)
	}

	lookAt(x,y,z){
		ObjectComunicator.C_lookAt(gameObject, x, y, z)
	}

	kill(){
	    ObjectComunicator.C_Kill(gameObject)
	}

	getPos(mode){
		return Vec3.new(getPosX(mode), getPosY(mode), getPosZ(mode))
	}

	getPosX(mode){
		return ObjectComunicator.C_getPosX(gameObject, mode)
	}

	getPosY(mode){
		return ObjectComunicator.C_getPosY(gameObject, mode)
	}

	getPosZ(mode){
		return ObjectComunicator.C_getPosZ(gameObject, mode)
	}

	getEuler(){
		return Vec3.new(getPitch(), getYaw(), getRoll())
	}

	getPitch(){
		return ObjectComunicator.C_getPitch(gameObject)
	}

	getYaw(){
		return ObjectComunicator.C_getYaw(gameObject)
	}

	getRoll(){
		return ObjectComunicator.C_getRoll(gameObject)
	}

	moveForward(speed){
		ObjectComunicator.C_MoveForward(gameObject, speed)
	}
}