
### Simple concurrent web server written in Golang

[go_web_server](./go_web_server)

I initially completed this as a coding challenge when applying to be an intern at a company called Ravelin. I thought it was interesting enough to include here. I had to learn Golang and implement a web server using standard libraries that handles concurrent requests and implements basic session handling. To support this, you have to use mutex's on the session store etc.
The front end is a simple form that sends information about the form's usage to the backend, which is then stored in the user session. I went on, after further technical interviews, to receive an offer from Ravelin.

The spec for the assignment follows below. My submission notes and in [notes.md](notes.md).

---

Ravelin Code Test
=================

## Summary
We need an HTTP server that will accept any POST request (JSON) from muliple clients' websites. Each request forms part of a struct (for that particular visitor) that will be printed to the terminal when the struct is fully complete. 

For the JS part of the test please feel free to use any libraries that may help you **but please only use the Go standard library for the backend**.

## Frontend (JS)
Insert JavaScript into the index.html (supplied) that captures and posts data every time one of the below events happens; this means you will be posting multiple times per visitor. Assume only one resize occurs.

  - if the screen resizes, the before and after dimensions
  - copy & paste (for each field)
  - time taken from the 1st character typed to clicking the submit button

### Example JSON Requests
```javascript
{
  "eventType": "copyAndPaste",
  "websiteUrl": "https://ravelin.com",
  "sessionId": "123123-123123-123123123",
  "pasted": true,
  "formId": "inputCardNumber"
}

{
  "eventType": "timeTaken",
  "websiteUrl": "https://ravelin.com",
  "sessionId": "123123-123123-123123123",
  "time": 72, // seconds
}

...

```

## Backend (Go)

The Backend must to:

1. Create a Server
2. Accept POST requests in JSON format similar to those specified above
3. Map the JSON requests to relevant sections of the data struct (specified below)
4. Print the struct for each stage of its construction
5. Also print the struct when it is complete (i.e. when the form submit button has been clicked)

### Go Struct
```go
type Data struct {
	WebsiteUrl         string
	SessionId          string
	ResizeFrom         Dimension
	ResizeTo           Dimension
	CopyAndPaste       map[string]bool // map[fieldId]true
	FormCompletionTime int // Seconds
}

type Dimension struct {
	Width  string
	Height string
}
```




