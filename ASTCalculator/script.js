window.onload = function(){
    // "Back end" Code
    // A Node represents either an Operator or an Operand
    var Node = function(text) {
        this.left = this.right = null;
        this.text = text;
    }

    Node.prototype.draw = function() {
        var interDraw = function(nod, parent) {		
            var disp = document.createElement("li");
            var text = document.createTextNode(nod.text);
            disp.appendChild(text);
            var ul = document.createElement("ul");
            parent.appendChild(disp);
            var dispParent = parent.appendChild(ul);
   		
            if (nod.left) interDraw(nod.left, dispParent);
            if (nod.right) interDraw(nod.right, dispParent);
        }	
        interDraw(this, document.getElementById("disproot"));	
    }	
	
    var Operator = function(op, left, right) {
        Node.apply(this, arguments);
        this.left = left;
        this.right = right;		
    }

    Operator.prototype = Object.create(Node.prototype);
    Operator.prototype.constructor = Operator;


    var Operand = function(number) {
        Node.apply(this, arguments);	
    }
	
    Operand.prototype = Object.create(Node.prototype);
    Operand.prototype.constructor = Operand;
	
    // Parser implement parsing algorithm --  recursive descent analysis
    var Parser = function(expression) {
        this.expr = expression;
        this.index = 0;	
        this.stack = [];
        this.nodeStack = [];
        this.result = 0;
        this.rootNode = null;
    }
	
    Parser.prototype.getResult = function() {
        return this.result;	
    }
	
    Parser.prototype.getRoot = function () {
        retNode = new Node(this.expr)
        retNode.left = this.rootNode;
        return retNode;	
    }
	
    /* EBNF
        <expression> := <term> {(+|-) <term>}
        <term> := <factor> {(*|/) <factor>}
        <factor> := <num> | (<expression>)	
    */
    Parser.prototype.run = function() {
        that = this;
		
        var nextChar = function() {
            that.index ++;
        }

        var expected = function(s) {
            throw "expect " + s;		
        }

        var matchToken = function(c) {
            if (that.expr[that.index] == c) {
                nextChar();			
            }		
        }

        var expression = function() {
            term();
            while (that.expr[that.index] in {'+': 1, '-': 1}) {
                that.nodeStack.push(that.rootNode);				
                that.stack.push(that.result);
                switch(that.expr[that.index])	{
                case '+':
                    matchToken('+');
                    add();
                    break;
                case '-':
                    matchToken('-');
                    subtract();
                    break;
                default:
                    expected("+|-");				
                }			
            }		
		}

        var add = function() {
            term();
            that.result = that.stack.pop() + that.result;
            that.rootNode = new Operator('+', that.nodeStack.pop(), that.rootNode);	
        }		

        var subtract = function() {
            term();
            that.result = that.stack.pop() - that.result;
            that.rootNode = new Operator('-', that.nodeStack.pop(), that.rootNode);	
        }

        var term = function() {
            factor();
            while (that.expr[that.index] in {'*': 1, '/': 1}) {
                that.nodeStack.push(that.rootNode);
                that.stack.push(that.result);
                switch(that.expr[that.index])	{
                case '*':
                    matchToken('*');
                    multiply();
                    break;
                case '/':
                    matchToken('/');
                    divide();
                    break;
                default:
                    expected("*|/");				
                }			
            }		
        }
		
        var multiply = function() {
            factor();
            that.result = that.stack.pop() * that.result;
            that.rootNode = new Operator('*', that.nodeStack.pop(), that.rootNode);
        }
		
        var divide = function() {
            factor();
            that.result = Math.floor(that.stack.pop() / that.result);
            that.rootNode = new Operator('/', that.nodeStack.pop(), that.rootNode);
        }

        var factor = function() {
            if (that.expr[that.index] == '(') {
                matchToken('(');
                expression();
                matchToken(')');
            } else {
                num();			
            }		
        }

        var num = function() {
            if (that.expr[that.index] < '0' || that.expr[that.index] > '9') {
                expected("digit");
            }
            var tmp = "";
            while (that.expr[that.index] >= '0' && that.expr[that.index] <= '9') {
                tmp += that.expr[that.index];
                nextChar();				
            }
            that.result = Number(tmp);
            that.rootNode = new Operand(that.result);			
        }
		
        // Call expression
        expression();
    }
	
    // User Interface Code
    var onclickButtons = function(button) {
        display = document.getElementsByClassName("display")[0];
        if (button.innerHTML == "C") {
            display.innerHTML = "";
            return;
        }
		
        if (button.innerHTML == "=") {
            parser = new Parser(display.innerHTML.replace(/<.*>/g, ""));
            parser.run();
            display.innerHTML = parser.getResult();      	
            parser.getRoot().draw();
            return;      
        }		
        
        if (display.innerHTML.length == 33) {
            alert('Cannot take more than 30 digits');
            return;
        }
        
        if (display.innerHTML.length == 15) {
            display.innerHTML += '<br/>';      
        }

        display.innerHTML += button.innerHTML;	
    };
	
    buttons = document.getElementsByClassName("button");
    Array.prototype.forEach.call(
        buttons, 
        function(button){
            button.onclick=function(){onclickButtons(button)};
        }
    );	
};
