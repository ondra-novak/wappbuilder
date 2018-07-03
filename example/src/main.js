
var App = function(){
	
	"use strict"

	window.onload=function() {
		console.log("Application started...!");
		if (!HashRouter.init()) {
			App.mainPage();
		}
	};

	function mainPage() {
		console.log("Entered main page...");		
	}
	
	
	return {
		"mainPage":mainPage
	};
	
	
	
}();
