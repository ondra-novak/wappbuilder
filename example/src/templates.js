
function loadTemplate(templateRef, templateTag) {
	"use strict";
	
	var templEl;
	var templName;
	if (typeof templateRef == "string") {
		templEl = document.getElementById(templateRef);
		templName = templateRef;
	} else {
		templEl = templateRef;
	}

	if (!templateTag) {
		if (templEl.dataset.tag) templateTag=templEl.dataset.tag; 
		else templateTag = "div";
	}

	
	var div = document.createElement(templateTag);
	if (templEl.dataset.class) {
		div.setAttribute("class", templEl.dataset.class);
	} else if (templName) {
		div.classList.add("templ_"+templName);				
	}
	
	if (templEl.dataset.style) {
		div.setAttribute("style", templEl.dataset.style);
	}
	
	if (templEl.content) {
		var imp = document.importNode(templEl.content,true);
		div.appendChild(imp);
	} else {
		var x = templEl.firstChild;
		while (x) {
			div.appendChild(x.cloneNode(true));
			x = x.nextSibling;
		}
	}
	return div;
};

var View = function(){
	"use strict";
		
	function View(elem) {
		if (typeof elem == "string") elem = document.getElementById(elem);
		this.root = elem;
		this.rebuildDataMap();
		
	};
	
	
	View.prototype.getRoot = function() {
		return this.root;
	}
	
	View.prototype.setContent = function(elem) {
		this.clearContent();
		this.root.appendChild(elem);
		this.rebuildDataMap();
	};
	
	View.clearContent = function(element) {
		var event = new Event("remove");
		var x =  element.firstChild
		while (x) {
			var y = x.firstSibling; 
			element.removeChild(x);
			x.dispatchEvent(event)
			x = y;
		}		
	}
	
	View.prototype.clearContent = function() {
		View.clearContent(this.root);
		this.byName = [];
	};
	
	View.prototype.markSelector = function(className) {
		var items = this.root.querySelectorAll(className);
		var cnt = items.length;
		for (var i = 0; i < cnt; i++) {
			items[i].classList.add("mark");
		}
	};
	
	View.prototype.unmark = function() {
		var items = this.root.querySelector("mark");
		var cnt = items.length;
		for (var i = 0; i < cnt; i++) {
			items[i].classList.remove("mark");
		}
	};
	
	View.prototype.installKbdHandler = function() {
		if (this.kbdHandler) return;
		this.kbdHandler = function(ev) {
			var x = ev.which || ev.keyCode;
			if (x == 13 && this.defaultAction) {
				if (this.defaultAction(this)) {
					ev.preventDefault();
					ev.stopPropagation();
				}
			} else if (x == 27 && this.cancelAction) {
				if (this.cancelAction(this)) {
					ev.preventDefault();
					ev.stopPropagation();
				}			
			}		
		}.bind(this);
		this.root.addEventListener("keydown", this.kbdHandler);
	};
	
	View.prototype.setDefaultAction = function(fn) {
		this.defaultAction = fn;
		this.installKbdHandler();
	};
	View.prototype.setCancelAction = function(fn) {
		this.cancelAction = fn;
		this.installKbdHandler();
	};
	
	View.prototype.installFocusHandler = function(fn) {
		if (this.focusHandler) return;
		this.focusHandler = function(ev) {
			if (this.firstTabElement) {
				setTimeout(function() {
					var c = document.activeElement;
					while (c) {
						if (c == this.root) return;
						c = c.parentElement;
					}
					this.firstTabElement.focus();
				}.bind(this),1);
			}
		}.bind(this);
		this.root.addEventListener("focusout", this.focusHandler);
	};
	
	View.prototype.setFirstTabElement = function(el) {
		this.firstTabElement = el;
		this.firstTabElement.focus();
		this.installFocusHandler();
	}
	
	View.prototype.setContentWithAnim = function(el, animParams) {
		
		var durations = animParams.duration;
		var enterClass = animParams.enterClass;
		var exitClass = animParam.exitClass;
		var root = this.root;
	
		function leave() {
			var cur = this.root.firstChild
			if (cur) {
				cur.classList.replace(enterClass,exitClass);
				cur.classList.add(enterClass);
				return new Promise(function(ok){
					setTimeout(function(){
						root.removeChild(cur);
						ok();
					}.bind(this),duration);
				}.bind(this));
			} else {
				return new Promise(function(ok){ok();});
			}
		}
		
		function enter() {
			el.classList.add(enterClass);
			root.appendChild(el);
			return new Promise(function(ok){
				setTimeout(ok,duration);
			});
		}
		
		if (this.nextPhase) {
			this.nextPhase = this.nextPhase.then(leave);
		} else {
			this.nextPhase = leave();	
		}
		this.nextPhase.then = this.nextPhase.then(enter);
	}
	
	View.prototype.rebuildDataMap = function() {
		this.byName = {};
		var placeholders = this.root.querySelectorAll("[data-name]");
		var i;
		var cnt = placeholders.length;
		for (i = 0; i < cnt; i++) {
			var name = placeholders[i].dataset.name;
			var lname = name.split(",");
			lname.forEach(function(x) {
				if (!(x in this.byName)) this.byName[x] = [];
				this.byName[x].push(placeholders[i]);
			}.bind(this));
		}
	}
	
	///Sets data in the view
	/**
	 * @data structured data. Promise can be used as value, the value is rendered when the promise
	 *  is resolved
	 *  
	 * @return function returns promise, which resolves after all promises are resolved. If there
	 * were no promise, function returns resolved promise
	 */
	View.prototype.setData = function(data) {
		var me = this;
		var finished = [];
		
		function processItem(itm, elemArr, val) {
				elemArr.forEach(function(elem) {
					if (elem) {
						if (typeof val == "object" && !Array.isArray(val)) {
							
							if (val)
							
							me.updateElementAttributes(elem,val);
							if (!("value" in val)) {
								return;
							}else {
								val = val.value;
							} 
						}
						var eltype = elem.tagName;
						if (elem.dataset.forceTag) eltype = elem.dataset.forceTag;			
						if (val)
						switch (eltype.toUpperCase()) {
							case "INPUT": me.updateInputElement(elem, val);break;
							case "SELECT": me.updateSelectElement(elem, val);break;
							case "TEXTAREA": me.updateInputElement(elem, val);break;
							case "TEMPLATE": me.updateTemplateElement(elem, val, finished);break;
							case "IMG": elem.setAttribute("src",val);break; 
							case "A": elem.setAttribute("href",val);break; 
							case "IFRAME": elem.setAttribute("src",val);break; 
							default: me.updateBasicElement(elem, val);break;
						}
					}
				});
			}
			
		
		
		for (var itm in data) {
			var elemArr = me.byName[itm];
			if (elemArr) {
				var val = data[itm];
				if (typeof val == "object" && (val instanceof Promise)) {
					finished.push(val.then(processItem.bind(this,itm,elemArr)));
				} else {
					processItem(itm,elemArr,val);
				}
			}
		}
		return Promise.all(finished);
	}
	
	View.prototype.updateElementAttributes = function(elem,val) {
		for (var itm in val) {
			if (itm == "value") continue;
			if (itm == "classList" && typeof val[itm] == "object") {
				for (var x in val[itm]) {
					if (val[itm][x]) elem.classList.add(x);
					else elem.classList.remove(x);
				}
			} else if (itm.substr(0,1) == "!") {
				var name = itm.substr(1);
				var fn = val[itm];
				if (!elem._t_eventHandlers) {
					elem._t_eventHandlers = {};
				}
				if (elem._t_eventHandlers && elem._t_eventHandlers[name]) {
					var reg = elem._t_eventHandlers[name];
					elem.removeEventListener(name,reg);
				}
				elem._t_eventHandlers[name] = fn;
				elem.addEventListener(name, fn);
			} else if (itm.substr(0,1) == ".") {
				var name = itm.substr(1);
				var v = val[itm];
				if (typeof v == "undefined") {
					delete elem[name];
				} else {
					elem[name] = v;
				}					
			} else if (val[itm]===null) {
				elem.removeAttribute(itm);
			} else {
				elem.setAttribute(itm, val[itm].toString())
			} 
		}
	}
	
	View.prototype.updateInputElement = function(elem, val) {
		var type = elem.getAttribute("type");
		if (type == "checkbox" || type == "radio") {
			if (typeof (val) == "boolean") {
				elem.checked = !(!val);
			} else if (typeof (val) == "object" && Array.isArray(val)) {
				elem.checked = val.indexOf(elem.value) != -1;
			} else if (typeof (val) == "string") {
				elem.checked = elem.value == val;
			} 
		} else {
			elem.value = val;
		}
	}
	
	
	View.prototype.updateSelectElement = function(elem, val) {
		if (typeof val == "object") {
			var curVal = elem.value;
			View.clearContent(elem);
			if (Array.isArray(val)) {
				var i = 0;
				var l = val.length;
				while (i < l) {
					var opt = document.createElement("option");
					opt.appendChild(document.createTextNode(val[i].toString()));
					w.appendChild(opt);
				}
			} else {
				for (var itm in val) {
					var opt = document.createElement("option");
					opt.appendChild(document.createTextNode(val[itm].toString()));
					opt.setAttribute("value",itm);
					w.appendChild(opt);				
				}
			}
			elem.value = curVal;
		} else {
			elem.value = val;
		}
	}
	
	View.prototype.updateTextArea = function(elem, val) {
		elem.value = val.toString();
	}
	
	View.prototype.updateBasicElement = function(elem, val) {
		View.clearContent(elem);
		if (typeof val == "object" && val instanceof Element) {
			elem.appendChild(val);
		} else {
			elem.appendChild(document.createTextNode(val));
		}
	}
	View.prototype.updateTemplateElement = function(elem, val) {
		
		var prev = elem.previousSibling;
		
		while (prev &&  prev.dataset && prev.dataset.containerItem) {
			prev.parentElement.removeChild(prev);
			prev = elem.previousSibling;
		}
		var cnt = val.length;
		for (var i = 0; i < cnt; i++) {
			
			var k = loadTemplate(elem);
			var v = new View(k);
			v.setData(val[i]);
			k.setAttribute("data-container-item","1");
			elem.parentElement.insertBefore(k, elem);
		}
		
	}
	
	View.prototype.readData = function(keys) {
		if (typeof keys == "undefined") {
			keys = Object.keys(this.byName);
		}
		var res = {};
		var me = this;
		keys.forEach(function(itm) {
			var elemArr = me.byName[itm];
			elemArr.forEach(function(elem){			
				if (elem) {
					var eltype = elem.tagName;
					if (elem.dataset.forceTag) eltype = elem.dataset.forceTag;
					switch (eltype.toUpperCase()) {
						case "INPUT": val = me.readInputElement(elem,res[itm]);break;
						case "SELECT": val = me.readSelectElement(elem,res[itm]);break;
						case "TEXTAREA": val = me.readTextAreaElement(elem,res[itm]);break;
						case "TEMPLATE": val = me.readTemplateElement(elem,res[itm]);break;
						default: val = me.readBasicElement(elem,res[itm]);break;
					}
					if (typeof val != "undefined") {
						res[itm] = val;
					}
				}
			});
		});
		return res;
	}
	
	View.prototype.readInputElement = function(elem, curVal) {
		var type = elem.getAttribute("type");
		if (type == "checkbox" || type == "radio") {
			if (typeof curVal == "undefined") {
				if (elem.checked) return elem.value;
				else return false;
			} else if (curVal === false) {
				if (elem.checked) return [elem.value];
				else return [];
			} else if (Array.isArray(curVal)) {
				if (elem.checked) return curVal.concat([elem.value]);
				else return curVal
			} else {
				if (elem.checked) return [curVal, elem.value];
				else return [curVal];
			}
		} else {
			return elem.value;
		}
	}
	View.prototype.readSelectElement = function(elem) {
		return elem.value;	
	}
	View.prototype.readTextAreaElement = function(elem) {
		return elem.value;	
	}
	View.prototype.readTemplateElement = function(elem) {
		var res = [];
		var prev = elem.previousSibling;
		
		if (prev && prev.dataset.container) {
			var x = prev.firstChild;
			while (x!=null) {
				var w = new View(x);
				res.push(w.readData());
				x = x.nextSibling;
			}
		} 
		return res;
	}
	
	View.prototype.readBasicElement = function(elem) {
		return elem.innerHTML;		
	}
}();

