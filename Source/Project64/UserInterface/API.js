Number.prototype.hex = function(len)
{
	len = (len || 8);
	var str = (this >>> 0).toString(16).toUpperCase()
	while (str.length < len)
	{
		str = "0" + str
	}
	return str
}

const u8 = 'u8', u16 = 'u16', u32 = 'u32',
	  s8 = 's8', s16 = 's16', s32 = 's32',
	  float = 'float',  double = 'double'

const _typeSizes = {
	u8: 1, u16: 2, u32: 4,
	s8: 1, s16: 2, s32: 4,
	float: 4, double: 8
}

const _regNums = {
	r0:  0, at:  1, v0:  2, v1:  3,
	a0:  4, a1:  5, a2:  6, a3:  7,
	t0:  8, t1:  9, t2: 10, t3: 11,
	t4: 12, t5: 13, t6: 14, t7: 15,
	s0: 16, s1: 17, s2: 18, s3: 19,
	s4: 20, s5: 21, s6: 22, s7: 23,
	t8: 24, t9: 25, k0: 26, k1: 27,
	gp: 28, sp: 29, fp: 30, ra: 31,

	f0:   0, f1:   1, f2:   2, f3:   3,
	f4:   4, f5:   5, f6:   6, f7:   7,
	f8:   8, f9:   9, f10: 10, f11: 11,
	f12: 12, f13: 13, f14: 14, f15: 15,
	f16: 16, f17: 17, f18: 18, f19: 19,
	f20: 20, f21: 21, f22: 22, f23: 23,
	f24: 24, f25: 25, f26: 26, f27: 27,
	f28: 28, f29: 29, f30: 30, f31: 31
}

function AddressRange(start, end)
{
	this.start = start >>> 0
	this.end = end >>> 0
	Object.freeze(this)
}

const ADDR_ANY = new AddressRange(0x00000000, 0x100000000)
const ADDR_ANY_KUSEG = new AddressRange(0x00000000, 0x80000000)
const ADDR_ANY_KSEG0 = new AddressRange(0x80000000, 0xA0000000)
const ADDR_ANY_KSEG1 = new AddressRange(0xA0000000, 0xC0000000)
const ADDR_ANY_KSEG2 = new AddressRange(0xC0000000, 0x100000000)
const ADDR_ANY_RDRAM = new AddressRange(0x80000000, 0x80800000)
const ADDR_ANY_RDRAM_UNC = new AddressRange(0xA0000000, 0xA0800000)
const ADDR_ANY_CART_ROM = new AddressRange(0x90000000, 0x96000000)
const ADDR_ANY_CART_ROM_UNC = new AddressRange(0xB0000000, 0xB6000000)

const system = {
	pause: function()
	{
		_native.pause()
	},
	resume: function()
	{

	},
	reset: function()
	{

	},
	hardreset: function()
	{

	},
	savestate: function()
	{

	},
	loadstate: function()
	{

	},
	setsaveslot: function(slot)
	{

	},
	getsaveslot: function()
	{

	},
	generatebitmap: function()
	{

	}
}

const debug = {
	showmemory: function(address)
	{
		
	},
	showcommands: function(address)
	{
		_native.showCommands(address)
	},
	breakhere: function()
	{
		debug.showcommands(gpr.pc)
		system.pause()
	},
	disasm: function()
	{

	}
}

const console = {
	print: function(data)
	{
		_native.consolePrint(data);
	},
	log: function()
	{
		for(var i in arguments)
		{
			console.print(arguments[i].toString())
			if(i < arguments.length - 1)
			{
				console.print(" ")
			}
		}
		console.print("\r\n")
	},
	clear: function()
	{
		_native.consoleClear();
	}
}

const screen = {
	print: function(x, y, text)
	{
		_native.screenPrint(x, y, text)
	}
}

const events = (function()
{
	var callbacks = {};
	var nextCallbackId = 0;
	return {
		on: function(hook, callback, param, param2, bOnce)
		{
			this._stashCallback(callback)
			return _native.addCallback(hook, callback, param, param2, bOnce)
		},
		onexec: function(addr, callback)
		{
			var param = 0;
			var param2 = 0;

			if (typeof (addr) == "object")
			{
				param = addr.start;
				param2 = addr.end;
			}
			else if (typeof (addr) == "number")
			{
				param = addr;
			}

			return events.on('exec', callback, param, param2)
		},
		onread: function(addr, callback)
		{
			var param = 0;
			var param2 = 0;

			if (typeof (addr) == "object")
			{
				param = addr.start;
				param2 = addr.end;
			}
			else if (typeof (addr) == "number")
			{
				param = addr;
			}

			return events.on('read', callback, param, param2)
		},
		onwrite: function(addr, callback)
		{
			var param = 0;
			var param2 = 0;

			if (typeof (addr) == "object")
			{
				param = addr.start;
				param2 = addr.end;
			}
			else if (typeof (addr) == "number")
			{
				param = addr;
			}

			return events.on('write', callback, param, param2)
		},
		ondraw: function(callback)
		{
			return events.on('draw', callback, 0)
		},
		remove: function(callbackId)
		{
			_native.removeCallback(callbackId)
		},
		clear: function(){},
		_stashCallback: function(callback)
		{
			callbacks[nextCallbackId] = callback
			return nextCallbackId++;
		},
		_unstashCallback: function()
		{
			
		},
	}
})();

const gpr = new Proxy({},
{
	get: function(obj, prop)
	{
		if (typeof prop == 'number' && prop < 32)
		{
			return _native.getGPRVal(prop)
		}
		if (prop in _regNums)
		{
			return _native.getGPRVal(_regNums[prop])
		}
		switch(prop)
		{
		    case 'pc': return _native.getPCVal(); break;
		    case 'hi': return _native.getHIVal(false); break;
		    case 'lo': return _native.getLOVal(false); break;
		}
	},
	set: function(obj, prop, val)
	{
		if (typeof prop == 'number' && prop < 32)
		{
			_native.setGPRVal(prop, false, val)
			return
		}
		if (prop in _regNums)
		{
			_native.setGPRVal(_regNums[prop], false, val)
			return
		}
		switch(prop)
		{
		    case 'pc': _native.setPCVal(val); break;
		    case 'hi': _native.setHIVal(false, val); break;
		    case 'lo': _native.setLOVal(false, val); break;
		}
	}
})

const ugpr = new Proxy({},
{
	get: function (obj, prop)
	{
		if (typeof prop == 'number' && prop < 32)
		{
			return _native.getGPRVal(prop, true)
		}
		if (prop in _regNums)
		{
			return _native.getGPRVal(_regNums[prop], true)
		}
		switch(prop)
		{
		    case 'hi': return _native.getHIVal(true); break;
		    case 'lo': return _native.getLOVal(true); break;
		}
	},
	set: function (obj, prop, val)
	{
		if (typeof prop == 'number' && prop < 32)
		{
			_native.setGPRVal(prop, true, val)
			return
		}
		if (prop in _regNums)
		{
			_native.setGPRVal(_regNums[prop], true, val)
			return
		}
		switch(prop)
		{
		    case 'hi': _native.setHIVal(true, val); break;
		    case 'lo': _native.setLOVal(true, val); break;
		}
	}
})

const fpr = new Proxy({},
{
	get: function(obj, prop)
	{
		if (typeof prop == 'number')
		{
			return _native.getFPRVal(prop)
		}
		if (prop in _regNums)
		{
			return _native.getFPRVal(_regNums[prop])
		}
	},
	set: function(obj, prop, val)
	{
		if (typeof prop == 'number' && prop < 32)
		{
			_native.setFPRVal(prop, false, val)
			return
		}
		if (prop in _regNums)
		{
			_native.setFPRVal(_regNums[prop], false, val)
		}
	}
})

const dfpr = new Proxy({},
{
    get: function (obj, prop)
    {
        if (typeof prop == 'number')
        {
            return _native.getFPRVal(prop, true)
        }
        if (prop in _regNums)
        {
            return _native.getFPRVal(_regNums[prop], true)
        }
    },
    set: function (obj, prop, val)
    {
        if (typeof prop == 'number' && prop < 32)
        {
            _native.setFPRVal(prop, true, val)
            return
        }
        if (prop in _regNums)
        {
            _native.setFPRVal(_regNums[prop], true, val)
        }
    }
})

const rom = {
	u8: new Proxy({},
	{
	    get: function (obj, prop)
	    {
			return _native.getROMInt(prop, 8, false)
		}
	}),
	u16: new Proxy({},
	{
	    get: function (obj, prop)
	    {
			return _native.getROMInt(prop, 16, false)
		}
	}),
	u32: new Proxy({},
	{
	    get: function (obj, prop)
	    {
			return _native.getROMInt(prop, 32, false)
		}
	}),
	s8: new Proxy({},
	{
	    get: function (obj, prop)
	    {
			return _native.getROMInt(prop, 8, true)
		}
	}),
	s16: new Proxy({},
	{
	    get: function (obj, prop)
	    {
			return _native.getROMInt(prop, 16, true)
		}
	}),
	s32: new Proxy({},
	{
	    get: function (obj, prop)
	    {
			return _native.getROMInt(prop, 32, true)
		}
	}),
	'float': new Proxy({},
	{
	    get: function (obj, prop)
	    {
			return _native.getROMFloat(prop)
		},
		set: function (obj, prop, val)
		{
			_native.setROMFloat(prop, val)
		}
	}),
	'double': new Proxy({},
	{
	    get: function (obj, prop)
	    {
			return _native.getROMFloat(prop, true)
		},
	    set: function (obj, prop, val)
	    {
			_native.setROMFloat(prop, val, true)
		}
	}),
	getblock: function (address, size)
	{
		return _native.getROMBlock(address, size)
	},
	getstring: function(address, maxLen)
	{
		return _native.getROMString(address, maxLen)
	},
}

const mem = {
	u8: new Proxy({},
	{
		get: function(obj, prop)
		{
			return _native.getRDRAMInt(prop, 8, false)
		},
		set: function(obj, prop, val)
		{
			_native.setRDRAMInt(prop, 8, val)
		}
	}),
	u16: new Proxy({},
	{
		get: function(obj, prop)
		{
			return _native.getRDRAMInt(prop, 16, false)
		},
		set: function(obj, prop, val)
		{
			_native.setRDRAMInt(prop, 16, val)
		}
	}),
	u32: new Proxy({},
	{
		get: function(obj, prop)
		{
			return _native.getRDRAMInt(prop, 32, false)
		},
		set: function(obj, prop, val)
		{
			_native.setRDRAMInt(prop, 32, val)
		}
	}),
	s8: new Proxy({},
	{
		get: function(obj, prop)
		{
			return _native.getRDRAMInt(prop, 8, true)
		},
		set: function(obj, prop, val)
		{
			_native.setRDRAMInt(prop, 8, val)
		}
	}),
	s16: new Proxy({},
	{
		get: function(obj, prop)
		{
			return _native.getRDRAMInt(prop, 16, true)
		},
		set: function(obj, prop, val)
		{
			_native.setRDRAMInt(prop, 16, val)
		}
	}),
	s32: new Proxy({},
	{
		get: function(obj, prop)
		{
			return _native.getRDRAMInt(prop, 32, true)
		},
		set: function(obj, prop, val)
		{
			_native.setRDRAMInt(prop, 32, val)
		}
	}),
	'float': new Proxy({},
	{
		get: function(obj, prop)
		{
			return _native.getRDRAMFloat(prop)
		},
		set: function(obj, prop, val)
		{
			_native.setRDRAMFloat(prop, val)
		}
	}),
	'double': new Proxy({},
	{
		get: function(obj, prop)
		{
			return _native.getRDRAMFloat(prop, true)
		},
		set: function(obj, prop, val)
		{
			_native.setRDRAMFloat(prop, val, true)
		}
	}),
	getblock: function(address, size)
	{
		return _native.getRDRAMBlock(address, size)
	},
	getstring: function(address, maxLen)
	{
	    return _native.getRDRAMString(address, maxLen)
	},
	bindvar: function(obj, baseAddr, name, type)
	{
		Object.defineProperty(obj, name,
		{
			get: function()
			{
				return mem[type][baseAddr]
			},
			set: function(val)
			{
				mem[type][baseAddr] = val
			}
		})
		return obj
	},
	bindvars: function(obj, list)
	{
		for(var i = 0; i < list.length; i++)
		{
			mem.bindvar(obj, list[i][0], list[i][1], list[i][2])
		}
		return obj
	},
	bindstruct: function(obj, baseAddr, props)
	{
		for (var name in props)
		{
			var type = props[name]
			var size = _typeSizes[type]
			mem.bindvar(obj, baseAddr, name, type)
			baseAddr += size
		}
		return obj
	},
	typedef: function(props, proto)
	{
		var size = 0
		for (var name in props)
		{
			size += _typeSizes[props[name]]
		}
		var StructClass = function(baseAddr)
		{
			mem.bindstruct(this, baseAddr, props)
		}
		StructClass.sizeof = function()
		{
			return size
		}
		/*if(proto)
		{
			StructClass.prototype = proto
		}*/
		return StructClass
	}
}

function alert(text, caption){
	caption = caption || ""
	_native.msgBox(text, caption)
}


function Socket(fd)
{
	var connected = false;
	
	var _fd;
	
	if(fd)
	{
		// assume this was constructed from Server
		_fd = fd;
		connected = true;
	} else {
		_fd = _native.sockCreate();
	}
	
	var _ondata = function(data){}
	var _onclose = function(){}
	var _onconnect = function(){}
	
	var _onconnect_base = function(){
		connected = 1;
		_onconnect();
	}
	
	var _bufferSize = 2048
	
	this.write = function(data, callback)
	{
		_native.write(_fd, data, callback)
	}
	
	this.close = function()
	{
		_native.close(_fd)
	}
	
	this.connect = function(settings, callback)
	{
		if(!connected)
		{
			_onconnect = callback;
			_native.sockConnect(_fd, settings.host || '127.0.0.1', settings.port || 80, _onconnect_base);
		}
	}
	
	var _read = function(data)
	{
		if(data == null)
		{
			connected = false;
			_onclose();
			return;
		}
		_native.read(_fd, _bufferSize, _read)
		_ondata(data)
	}
	
	this.on = function(eventType, callback)
	{
		switch(eventType)
		{
		case 'data':
			_ondata = callback
			_native.read(_fd, _bufferSize, _read)
			break;
		case 'close':
			// note: does nothing if ondata not set
			_onclose = callback
			break;
		}
	}
}

function Server(settings)
{
	var _this = this;
	var _fd = _native.sockCreate()
	
	var _onconnection = function(socket){}
	
	if(settings.port)
	{
		_native.sockListen(_fd, settings.port || 80)
	}
	
	// Intermediate callback
	//  convert clientFd to Socket and accept next client
	var _acceptClient = function(clientFd)
	{
		_onconnection(new Socket(clientFd))
		_native.sockAccept(_fd, _acceptClient)
	}
	
	this.listen = function(port)
	{
		_native.sockListen(_fd, port)
	}
	
	this.on = function(eventType, callback)
	{
		switch(eventType)
		{
		case 'connection':
			_onconnection = callback
			_native.sockAccept(_fd, _acceptClient)
			break;
		}
	}
}
