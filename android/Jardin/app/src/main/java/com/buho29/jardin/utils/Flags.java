package com.buho29.jardin.utils;
import android.util.Log;

public class Flags {

	static String log = "flags";
	
	public static void testAlarm(){
		int lunes = 	 1;// 0000001
		int martes = 	 2;// 0000010
		int miercoles =  4;// 0000100
		int jueves = 	 8;// 0001000
		int viernes = 	16;// 0010000
		int sabado = 	32;// 0100000
		int domingo = 	64;// 1000000
		
		Flags alarm = new Flags(lunes|martes|miercoles|jueves|viernes);
		
		if(alarm.contain(martes))
			Log.d(log, "hay alarma el marte");
		if(alarm.check(sabado|domingo))
			Log.d(log, "hay alarmas el sabado y el domingo");
		
		if(alarm.contain(sabado|domingo))
			Log.d(log, "hay alarma el sabado o el domingo o ambas");
		
		alarm.remove(martes);
		if(!alarm.contain(martes))
			Log.d(log, "no hay alarma el martes");
		
		alarm.remove(jueves|domingo);
		
		if(alarm.check(lunes|miercoles|viernes))
			Log.d(log, "hay 3 alarmas el lunes|miercoles|viernes");
		
		alarm.add(domingo);
		if(alarm.contain(domingo))
			Log.d(log, "toca misa!");

		//ejemplo 2

		boolean[] pins = {true,false,false,true};

		Flags f = new Flags();
		for(int i = 0 ;i<pins.length;i++){
			if(pins[i]){
				f.add((int) Math.pow(2,i));
			}
		}
		Log.e("teta",f.get()+" - "+Integer.toBinaryString(f.get()));//teta: 9 - 1001
	}
	
	private int mFlags = 0;
	
	public Flags(){}
	
	/**
	 * pone mFlags a 0
	 */
	public void reset(){
		mFlags = 0;
	}
	
	/**
	 * agrega un flags
	 * @param flags
	 */
	public void add(int flags){
		mFlags |= flags;
	}
	
	/**
	 * elimina un flags
	 * @param flags
	 */
	public void remove(int flags){
		// 1000110 mFlags
		// 0010110 flags
		// 0000110 mFlags & flags , elimina las q no estan encendidas en mFlags 
		// 1000000 0000110 ^ mflags 
		
		mFlags ^= (mFlags & flags);
	}
	
	/**
	 * comparacion 
	 * @param flags
	 * @return devuelve si cuando existe algunos de los flags
	 */
	public boolean contain(int flags){
		return (mFlags & flags) > 0;
	}
	/**
	 * comparacion estricta
	 * @param flags
	 * @return devuelve si cuando existe todos los flags
	 */
	public boolean check(int flags){
		return (mFlags & flags) == flags;
	}
	
	public Flags(int flags){
		mFlags = flags;
	}
	public void set(int flags){
		mFlags = flags;
	}
	/**
	 * 
	 * @return mFlags
	 */
	public int get(){
		return mFlags;
	}
	
	@Override
	public boolean equals(Object o) {
		Flags f = (Flags) o;
		return mFlags == f.get();
	}
	
	public String toString(){
		return Integer.toBinaryString(mFlags);
	}
}
