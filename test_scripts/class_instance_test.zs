class ClassInstanceTest()
{

    print("creating an instance of Hello class...");
    var helloObject = new Hello();
    var message = helloObject.hello_message;
    print("hello object's message is " + message);
    print("now set it to osman...");
    helloObject.hello_message = "osman is awesome!";
    print("hello object's message is " + helloObject.hello_message);
    print("now call sayHello on helloObject...");
    helloObject.sayHello();
    print("now set it vie setter method");
    helloObject.setHelloMessage("osman again!");
    helloObject.sayHello();

}