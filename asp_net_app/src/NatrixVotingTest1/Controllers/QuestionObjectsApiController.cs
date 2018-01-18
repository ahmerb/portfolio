using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using Microsoft.AspNetCore.Http;
using Microsoft.AspNetCore.Mvc;
using NatrixVotingTest1.Interfaces;
using NatrixVotingTest1.Models;

namespace NatrixVotingTest1.Controllers
{
    [Route("api/[controller]")]
    public class QuestionObjectsController : Controller
    {
        private readonly INatrixVotingTest1Repository _repo;


        public QuestionObjectsController(INatrixVotingTest1Repository repo)
        {
            _repo = repo;
        }


        [HttpGet]
        public IActionResult List()
        {
            return Ok(_repo.All);
        }


        [HttpPost("{id}")]
        public IActionResult Create(string id, [FromBody]QuestionObject qObj)
        {
            try
            {
                if (qObj == null || !ModelState.IsValid)
                {
                    return BadRequest(ErrorCode.InvalidQuestionObject.ToString());
                }
                bool qObjExists = _repo.DoesItemExist(qObj.ID);
                if (qObjExists)
                {
                    return StatusCode(StatusCodes.Status409Conflict, ErrorCode.QuestionObjectIDInUse.ToString());
                }
                _repo.Insert(qObj);
            }
            catch (Exception)
            {
                return BadRequest(ErrorCode.ErrorCreatingQuestionObject.ToString());
            }
            return Ok(qObj); // return newly created item in HTTP response
        }


        [HttpPut("{id}")]
        public IActionResult Edit(string id, [FromBody] QuestionObject qObj)
        {
            try
            {
                if(qObj == null || !ModelState.IsValid)
                {
                    return BadRequest(ErrorCode.InvalidQuestionObject.ToString());
                }
                var existingItem = _repo.Find(id);
                if(existingItem == null)
                {
                    return NotFound(ErrorCode.QuestionObjectNotFound.ToString());
                }
                _repo.Update(qObj);
            }
            catch(Exception)
            {
                return BadRequest(ErrorCode.ErrorUpdatingQuestionObject.ToString());
            }
            return NoContent();
        }


        [HttpDelete("{id}")]
        public IActionResult Delete(string id)
        {
            try
            {
                var item = _repo.Find(id);
                if(item == null)
                {
                    return NotFound(ErrorCode.QuestionObjectNotFound.ToString());
                }
                _repo.Delete(id);
            }
            catch(Exception)
            {
                return BadRequest(ErrorCode.ErrorDeletingQuestionObject.ToString());
            }
            return NoContent();
        }
    }


    public enum ErrorCode
    {
        InvalidQuestionObject,
        QuestionObjectIDInUse,
        QuestionObjectNotFound,
        ErrorCreatingQuestionObject,
        ErrorUpdatingQuestionObject,
        ErrorDeletingQuestionObject
    }
}
