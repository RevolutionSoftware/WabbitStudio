﻿using System;
using System.Collections.Generic;
using Revsoft.Wabbitcode.Services.Project.Interface;

namespace Revsoft.Wabbitcode.Services.Project
{
	public class BuildConfig : IBuildConfig, ICloneable
	{
		private string name;
		public string Name
		{
			get { return name; }
		}

		List<IBuildStep> steps = new List<IBuildStep>();
		public List<IBuildStep> Steps
		{
			get { return steps; }
		}

		IProject project;
		public IProject Project
		{
			get { return project; }
		}

		public BuildConfig(IProject project, string name)
		{
			this.name = name;
			this.project = project;
		}

		public void SortSteps()
		{
			steps.Sort(SortSteps);
		}

		public void Build()
		{
			SortSteps();
			Project.ProjectOutputs.Clear();
			Project.ListOutputs.Clear();
			Project.LabelOutputs.Clear();
            foreach (IBuildStep step in steps)
            {
                step.Build();
            }
		}

		static int SortSteps(IBuildStep step1, IBuildStep step2)
		{
			if (step1 == null)
			{
                if (step2 == null)
                {
                    return 0;
                }
                else
                {
                    return -1;
                }
			}
			else
			{
                if (step2 == null)
                {
                    return 1;
                }
                if (step1.StepNumber == step2.StepNumber)
                {
                    return 0;
                }
                if (step1.StepNumber > step2.StepNumber)
                {
                    return 1;
                }
                else
                {
                    return -1;
                }
			}
		}

		public override string ToString()
		{
			return name;
		}

		public override int GetHashCode()
		{
			return name.GetHashCode() +  48 * steps.Count.GetHashCode();
		}

		public override bool Equals(object obj)
		{
            if (!(obj is BuildConfig))
            {
                return false;
            }
			BuildConfig config = (BuildConfig)obj;
            if (config.name == this.name && config.steps.Count == this.steps.Count)
            {
                return true;
            }
            else
            {
                return false;
            }
		}

		public object Clone()
		{
			BuildConfig clone = new BuildConfig(project, this.name);
			clone.steps = new List<IBuildStep>();
            clone.steps.AddRange(steps);
			return clone;
		}
	}
}
