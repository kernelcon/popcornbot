//require filesystem for writing state
var fs = require("fs");
//keep track of state in memory
var popCount = [{ machine: "A", count: "0" }, { machine: "B", count: "0" }];

//import auth token
var AUTH_TOKEN = process.env.API_AUTH_TOKEN;

//import slack
var SLACK_WEBHOOK = process.env.SLACK_WEBHOOK;
var slack = require("slack-notify")(SLACK_WEBHOOK);

//slack shortcut function
function slackmsg(msg) {
  slack.send({
    text: msg
  });
}

//loads states from filesystem into memory when the project restarts
function loadFileData() {
  //Assuming machines are "A" and "B", we should make defs for these
  fs.readFile(".data/A.json", function(err, data) {
    if (err) return console.log(err);
    var result = JSON.parse(data);
    popCount[result.machine] = result.count;
  });
  fs.readFile(".data/B.json", function(err, data) {
    if (err) return console.log(err);
    var result = JSON.parse(data);
    popCount[result.machine] = result.count;
  });
}

loadFileData();


var routes = function(app) {
  //
  // defaut response to somebody who goes to popcornapi.gitch.me
  //
  app.get("/", function(req, res) {
    res.send("<h1>popcorn!</h1>");
    console.log("Received GET");
  });

  //
  // POST request to /popcorn will update everything.  this is where danger be
  //
  app.post("/popcorn", function(req, res) {
    //we need to do input validation on req.body so somebody doesn't hax us
    if (!req.body.machine || !req.body.count || req.body.auth != AUTH_TOKEN) {
      console.log("Received incomplete POST: " + JSON.stringify(req.body));
      return res.send({ status: "error", message: "missing parameter(s)" });
    } else {
      console.log("Received POST: " + JSON.stringify(req.body));
      popCount[req.body.machine] = req.body.count;
      fs.writeFileSync(
        ".data/" + req.body.machine + ".json",
        JSON.stringify(req.body)
      );
      slackmsg(
        "*POPCORN SLAVE!* MACHINE " +
          req.body.machine +
          " NEEDS MOAARR POPCORN! _(" +
          req.body.count +
          " requests)_"
      );
      return res.send(req.body);
    }
  });

  //
  // GET requests return the current state.  no auth required.  this is used when the machines boot and for dashboards
  // can probably combine these if I get smart
  app.get("/popcorn/A", function(req, res) {
    var dummyData = {
      machine: "A",
      count: popCount["A"]
    };
    console.log("Received GET: " + JSON.stringify(req.body));
    return res.send(dummyData);
  });

  app.get("/popcorn/B", function(req, res) {
    var dummyData = {
      machine: "B",
      count: popCount["B"]
    };
    console.log("Received GET: " + JSON.stringify(req.body));
    return res.send(dummyData);
  });
};

module.exports = routes;