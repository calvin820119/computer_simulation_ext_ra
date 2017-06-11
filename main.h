#ifndef _EXT_RA_
#define _EXT_RA_



typedef enum rar_e{
	conventional=1,
	ext_rar_2=2,
	ext_rar_3=3,
	ext_rar_4=4
}rar_t;

typedef enum events_e{
	event_ra_period,
	event_stop,
	num_normal_event
}events_t;

typedef struct ue_s{
    float arrival_time;
    int preamble_index;
    int is_active;
    int retransmit_counter;
    int backoff_counter;
    struct ue_s *next;
}ue_t;

typedef struct preamble_s{
    int num_selected_rar[4];
	ue_t *rar_ue_list[4];
}preamble_t;

    

typedef struct ext_ra_inst_s{
    
    int total_ras;
    int number_of_preamble;
    int num_ue;
    float mean_interarrival;
    float ra_period;
    rar_t rar_type;
    ue_t *ue_list;//[1000];
    int max_retransmit;
    preamble_t *preamble_table;//[30];
    int back_off_window_size;
    int ras;
    int failed;
    int attempt;
    int success;
    int collide;
    int once_attempt_success;
    int retransmit;
    int once_attempt_collide;
    int trial;

}ext_ra_inst_t;


float exponetial(float mean);
void ue_backoff_process(ext_ra_inst_t *inst);
void ra_procedure(ext_ra_inst_t *inst);
void ue_selected_preamble(ext_ra_inst_t *inst, int ue_id);
void ue_arrival(ext_ra_inst_t *inst, int next_event_type);
void initialize(ext_ra_inst_t *inst);
void initialize_ue_preamble(ext_ra_inst_t *inst);
void timing(ext_ra_inst_t *inst); 
void report(ext_ra_inst_t *inst);
#endif
