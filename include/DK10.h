class DK10State;

class DK10Context{
    private:
        DK10State *state_;
    public:
        DK10Context(DK10State *dk10State) : state_(nullptr){
            this->TransitionTo(state_);
        };
        
        void TransitionTo(DK10State *state) {
            if (this->state_ != nullptr)
                delete this->state_;
            this->state_ = state;
            this->state_->set_context(this);
        };
        void tick(){
            this->state_->tick();
        };

};

class DK10State{
    protected: 
        DK10Context *context_;
    public:
        void set_context (DK10Context *context){
            this->context_ = context;
        };
        virtual void tick();
        virtual void doStart();
        virtual void doStop();
        virtual void doPause();
        virtual void doStep();
};

class DK10StateStop : public DK10State {
    public:
        virtual void tick() override;
        virtual void doStart() override;
        virtual void doStop() override;
        virtual void doPause() override;
        virtual void doStep() override;
};

class DK10StateReady: public DK10State {
    public:
        virtual void tick() override{

        };
        virtual void doStart() override{
            this->context_->TransitionTo(new DK10StateLoading1); 
        };
        virtual void doStop() override;
        virtual void doPause() override;
        virtual void doStep() override;
};

class DK10StateLoading1 : public DK10State {
    public:
        virtual void tick() override{
            //открыть клапан
            //следить за весом
            //когда вес набран
                //закрыть клапан
            
            //проверка глобального режима 
            Serial.print(typeid(this->context_->state_).name());
        };
        virtual void doStart() override;
        virtual void doStop() override{
            this->context_->TransitionTo(new DK10StatePause);
        };
        virtual void doPause() override;
        virtual void doStep() override;
};

class DK10StateLoading2 : public DK10State {

};

class DK10StateUnload : public DK10State {

};

class DK10StatePause : public DK10State {

};

