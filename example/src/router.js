
var HashRouter = (function(){

	"use strict";
	

	return {
		curHandler:null,
		userHandlers:{},
		staticHandlers:{},
		curReq: null,		
		
		reg:function(kw, cb) {
			this.userHandlers[kw] = cb;									
		},
		regStatic: function(kw, cb) {
			this.staticHandlers[kw] = cb;
		},

		unreg: function(kv) {
			delete this.userHandlers[kv];
		},

		unregStatic: function(kv) {
			delete this.staticHandlers[kv];
		},

		init: function(object) {
			
			function handler() {
				
				var h = location.hash;
				if (!h) return false;
				var data = h.substr(1);
				try {
					if (this.staticHandlers[data]) {
						this.staticHandlers[data].apply(object);
						return true;
					} else {
						var bdata = atob(data);
						var jdata = JSON.parse(bdata);
						this.curReq = jdata;
						var hndl = jdata[0];
						var params = jdata[1];
						this.userHandlers[hndl].apply(object, params)
						return true;
					}
				} catch (e) {
					console.log(e);
					return false;
				}
			}
			
			if (this.curHandler) window.removeEventListener("hashchange", this.curHandler)
			this.curHandler = function() {handler.call(this);}.bind(this);
			window.addEventListener("hashchange", this.curHandler);
			return handler.call(this);
		},
		
		linkv: function(name, data) {
			
			var obj = [name, data];
			var lnk = JSON.stringify(obj);
			var blnk = btoa(lnk);
			return "#"+blnk;
			
		},
		
		link: function(name /*,....args...*/) {
			var args = Array.prototype.slice.call(arguments, 1);
			return this.linkv(name, args);
		},
		
		clear: function() {
			this.unreg();
			this.userHandlers = {};
			this.staticHandlers={};
		},
		
		navigatev: function(name, data) {
			var lnk = this.linkv(name,data);
			location.hash = lnk;
		},
		
		navigate: function(name /*, args */) {
			var args = Array.prototype.slice.call(arguments, 1);
			this.navigatev(name, args);			
		},
		navigateStatic: function(name) {
			location.hash = "#"+name;
		},
		
		hlink:function(name /*, args */) {
			return this.link.apply(hashRouter,arguments);
		},
		ahref:function(name /*, args */) {
			var el = document.createElement("a");
			el.setAttribute("href", this.hlink.apply(null, arguments));
			return el;
		}
	}
		
})();

