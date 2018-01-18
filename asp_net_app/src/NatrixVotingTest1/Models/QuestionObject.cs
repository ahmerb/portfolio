using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using System.ComponentModel.DataAnnotations;

namespace NatrixVotingTest1.Models
{
    public class QuestionObject
    {
        [Required]
        public string ID { get; set; }

        [Required]
        public string Question { get; set; }

        [Required]
        public string AnswerA { get; set; }

        [Required]
        public string AnswerB { get; set; }

        public int AnswerAVote { get; set; }

        public int AnswerBVote { get; set; }
    }
}
