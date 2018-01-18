using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading.Tasks;
using NatrixVotingTest1.Interfaces;
using NatrixVotingTest1.Models;


namespace NatrixVotingTest1.Services
{
    public class NatrixVotingTest1Repository : INatrixVotingTest1Repository
    {
        private List<QuestionObject> _qObjList;


        public NatrixVotingTest1Repository()
        {
            InitData();
        }


        public IEnumerable<QuestionObject> All
        {
            get { return _qObjList; }
        }

        public bool DoesItemExist(string id)
        {
            return _qObjList.Any(item => item.ID == id);
        }

        public QuestionObject Find(string id)
        {
            return _qObjList.FirstOrDefault(item => item.ID == id);
        }

        public void Insert(QuestionObject qObj)
        {
            _qObjList.Add(qObj);
        }

        public void Update(QuestionObject qObj)
        {
            var qObjItem = this.Find(qObj.ID);
            var index = _qObjList.IndexOf(qObjItem);
            _qObjList.RemoveAt(index);
            _qObjList.Insert(index, qObj);
        }

        public void Delete(string id)
        {
            _qObjList.Remove(this.Find(id));
        }

        private void InitData()
        {
            // init list

            _qObjList = new List<QuestionObject>();

            // seed data

            var qObj1 = new QuestionObject
            {
                ID = "1",
                Question = "Vim or Emacs?",
                AnswerA = "Vim",
                AnswerB = "Emacs",
                AnswerAVote = 2,
                AnswerBVote = 3
            };

            var qObj2 = new QuestionObject
            {
                ID = "2",
                Question = "Weak or strong typing?",
                AnswerA = "Weak",
                AnswerB = "Strong",
                AnswerAVote = 0,
                AnswerBVote = 100
            };

            _qObjList.Add(qObj1);
            _qObjList.Add(qObj2);
        }
    }
}
