#encoding: utf-8

require 'spec_helper'
require 'jailman/configuration'

describe Jailman::Configuration do
  let(:jail) {Jailman::Jail.new('my_jail')}

  context 'when initialized' do
    context 'and no jail passed as argument' do

      it 'raises an Exception' do
        expect{ described_class.new }.to raise_error('A jail must be passed')
      end

    end

    context 'when a jail is filled' do

      it 'populates jail' do
        config  = described_class.new(jail)

        expect(config.jail).to_not be_nil
        expect(config.jail).to be(jail)
      end

    end

  end

  context 'when creating a config file' do
    let(:config) do
      template = <<-YAML
        application:
          name : <%= @jail.name %>
          repository: # git@github.com:tonyfabeen/jailman.git
        run:
          commands:
            # Here you put your list of commands
            - echo testing
            - free -m
      YAML

      described_class.any_instance.stub(:config_template) {template}
      config = described_class.new(jail_factory)
      config
    end

    before { FileUtils.mkdir_p(config.jail.directory) }

    context 'on create' do

      it 'creates' do
        config.create_yaml

        yaml_path    = config.jail.directory + "/jail.yml"
        yaml_data = YAML.load_file(yaml_path)

        expect(File.exists?(yaml_path)).to be_true
        expect(yaml_data).to_not be_nil
        expect(yaml_data["application"]["name"]).to match(config.jail.name)
      end

    end

    context 'on read' do

      it 'reads' do

        config.create_yaml

        yaml_path    = config.jail.directory + "/jail.yml"
        yaml_data = YAML.load_file(yaml_path)

        config.read

        expect(config.application_name).to match(yaml_data["application"]["name"])
        expect(config.commands).to_not be_nil
        expect(config.commands.first).to match("echo testing")
      end

    end

    after { FileUtils.rm_rf(config.jail.directory) }

  end



end
